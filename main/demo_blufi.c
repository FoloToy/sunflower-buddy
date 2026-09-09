#include "demo_blufi.h"
#include "demo_blufi_security.h"

#include "esp_blufi.h"
#include "esp_blufi_api.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PROVISIONING_QUEUE_LENGTH = 16,
    PROVISIONING_TASK_STACK_SIZE = 6144,
    WIFI_SCAN_RESULT_LIMIT = 20,
    WIFI_RETRY_INITIAL_MS = 1000,
    WIFI_RETRY_MAX_MS = 30000,
    // BLUFI carries its payload length in one byte. Capping the negotiated
    // GATT MTU prevents clients from emitting an oversized, wrapped frame.
    BLUFI_GATT_LOCAL_MTU = 255,
};

typedef enum {
    COMMAND_WIFI_STARTED,
    COMMAND_WIFI_CONNECTED,
    COMMAND_WIFI_DISCONNECTED,
    COMMAND_WIFI_GOT_IP,
    COMMAND_RETRY_CONNECT,
    COMMAND_CONNECT_PENDING,
    COMMAND_DISCONNECT,
    COMMAND_SET_SSID,
    COMMAND_SET_PASSWORD,
    COMMAND_SET_BSSID,
    COMMAND_START_ADVERTISING,
    COMMAND_REPORT_STATUS,
    COMMAND_START_SCAN,
    COMMAND_REPORT_SCAN,
    COMMAND_RESET_PROVISIONING,
} provisioning_command_type_t;

typedef struct {
    provisioning_command_type_t type;
    union {
        struct {
            uint8_t bytes[64];
            uint8_t length;
        } data;
        struct {
            uint8_t reason;
            int8_t rssi;
        } disconnected;
        esp_ip4_addr_t ip;
    } value;
} provisioning_command_t;

static const char *TAG = "demo_blufi";
static QueueHandle_t s_commands;
static esp_timer_handle_t s_retry_timer;
static wifi_config_t s_pending_config;
static char s_device_name[24];
static uint32_t s_retry_delay_ms = WIFI_RETRY_INITIAL_MS;
static uint8_t s_last_disconnect_reason;
static int8_t s_last_disconnect_rssi = -128;
static bool s_reconfigure_after_disconnect;
static atomic_bool s_initialized = ATOMIC_VAR_INIT(false);
static atomic_bool s_has_credentials = ATOMIC_VAR_INIT(false);
static atomic_bool s_wifi_started = ATOMIC_VAR_INIT(false);
static atomic_bool s_station_connected = ATOMIC_VAR_INIT(false);
static atomic_bool s_has_ip = ATOMIC_VAR_INIT(false);
static atomic_bool s_connecting = ATOMIC_VAR_INIT(false);
static atomic_bool s_manual_disconnect = ATOMIC_VAR_INIT(false);
static atomic_bool s_provisioning = ATOMIC_VAR_INIT(false);
static atomic_bool s_blufi_ready = ATOMIC_VAR_INIT(false);
static atomic_bool s_ble_connected = ATOMIC_VAR_INIT(false);
static atomic_bool s_advertising = ATOMIC_VAR_INIT(false);

static void blufi_event_handler(esp_blufi_cb_event_t event,
                                esp_blufi_cb_param_t *parameter);
static void schedule_reconnect(void);

static esp_blufi_callbacks_t s_blufi_callbacks = {
    .event_cb = blufi_event_handler,
    .negotiate_data_handler = demo_blufi_negotiate_data,
    .encrypt_func = demo_blufi_encrypt,
    .decrypt_func = demo_blufi_decrypt,
    .checksum_func = demo_blufi_checksum,
};

static size_t station_ssid_length(const wifi_config_t *config) {
    size_t length = 0;
    while (length < sizeof(config->sta.ssid) && config->sta.ssid[length]) ++length;
    return length;
}

static const char *wifi_disconnect_reason_name(uint8_t reason) {
    switch (reason) {
        case WIFI_REASON_AUTH_EXPIRE:
            return "authentication expired";
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            return "4-way handshake timeout";
        case WIFI_REASON_BEACON_TIMEOUT:
            return "beacon timeout";
        case WIFI_REASON_NO_AP_FOUND:
            return "access point not found";
        case WIFI_REASON_AUTH_FAIL:
            return "authentication failed";
        case WIFI_REASON_ASSOC_FAIL:
            return "association failed";
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            return "handshake timeout";
        case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
            return "no access point with compatible security";
        case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
            return "access point below authentication threshold";
        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
            return "access point below RSSI threshold";
        default:
            return "unspecified";
    }
}

static bool post_command(const provisioning_command_t *command) {
    return s_commands && xQueueSend(s_commands, command, 0) == pdTRUE;
}

static void post_simple_command(provisioning_command_type_t type) {
    const provisioning_command_t command = {.type = type};
    if (!post_command(&command)) {
        ESP_LOGW(TAG, "dropping provisioning command %d", type);
    }
}

static void stop_retry_timer(void) {
    if (!s_retry_timer) return;
    const esp_err_t error = esp_timer_stop(s_retry_timer);
    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "cannot stop reconnect timer: %s", esp_err_to_name(error));
    }
}

static esp_err_t ensure_wifi_started(void) {
    if (atomic_load(&s_wifi_started)) return ESP_OK;

    const esp_err_t error = esp_wifi_start();
    if (error == ESP_OK) return ESP_OK;
    if (error == ESP_ERR_WIFI_NOT_STOPPED) {
        atomic_store(&s_wifi_started, true);
        return ESP_OK;
    }
    return error;
}

static void stop_advertising(void) {
    if (!atomic_exchange(&s_advertising, false)) return;
    esp_blufi_adv_stop();
}

static void start_advertising(void) {
    if (!atomic_load(&s_blufi_ready) || !atomic_load(&s_provisioning) ||
        atomic_load(&s_ble_connected) || atomic_exchange(&s_advertising, true)) {
        return;
    }
    ESP_LOGI(TAG, "BLUFI provisioning available as %s", s_device_name);
    esp_blufi_adv_start_with_name(s_device_name);
}

static void report_blufi_status(void) {
    if (!atomic_load(&s_ble_connected)) return;

    wifi_mode_t mode = WIFI_MODE_STA;
    esp_blufi_extra_info_t info = {0};
    wifi_config_t config = {0};
    wifi_ap_record_t access_point = {0};
    esp_blufi_sta_conn_state_t state = ESP_BLUFI_STA_CONN_FAIL;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_get_mode(&mode));
    if (atomic_load(&s_station_connected) &&
        esp_wifi_sta_get_ap_info(&access_point) == ESP_OK) {
        memcpy(info.sta_bssid, access_point.bssid, sizeof(info.sta_bssid));
        info.sta_bssid_set = true;
        if (esp_wifi_get_config(WIFI_IF_STA, &config) == ESP_OK) {
            info.sta_ssid = config.sta.ssid;
            info.sta_ssid_len = station_ssid_length(&config);
        }
        state = atomic_load(&s_has_ip) ? ESP_BLUFI_STA_CONN_SUCCESS
                                       : ESP_BLUFI_STA_NO_IP;
    } else if (atomic_load(&s_connecting)) {
        state = ESP_BLUFI_STA_CONNECTING;
    } else {
        info.sta_conn_end_reason_set = true;
        info.sta_conn_end_reason = s_last_disconnect_reason;
        info.sta_conn_rssi_set = true;
        info.sta_conn_rssi = s_last_disconnect_rssi;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        esp_blufi_send_wifi_conn_report(mode, state, 0, &info));
}

static void connect_station(void) {
    if (!atomic_load(&s_has_credentials) ||
        atomic_load(&s_manual_disconnect)) {
        return;
    }
    const esp_err_t error = esp_wifi_connect();
    atomic_store(&s_connecting, error == ESP_OK);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi connect request failed: %s", esp_err_to_name(error));
        schedule_reconnect();
    } else {
        ESP_LOGI(TAG, "Wi-Fi connection attempt started");
    }
    report_blufi_status();
}

static void schedule_reconnect(void) {
    if (!atomic_load(&s_has_credentials) ||
        atomic_load(&s_manual_disconnect)) {
        return;
    }

    stop_retry_timer();
    const uint32_t delay_ms = s_retry_delay_ms;
    const esp_err_t error =
        esp_timer_start_once(s_retry_timer, (uint64_t)delay_ms * 1000);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "cannot schedule Wi-Fi reconnect: %s",
                 esp_err_to_name(error));
        return;
    }
    ESP_LOGW(TAG,
             "Wi-Fi disconnected: %s (reason=%u, rssi=%d); retrying in %lu ms",
             wifi_disconnect_reason_name(s_last_disconnect_reason),
             s_last_disconnect_reason, (int)s_last_disconnect_rssi,
             (unsigned long)delay_ms);
    s_retry_delay_ms = delay_ms >= WIFI_RETRY_MAX_MS / 2
                           ? WIFI_RETRY_MAX_MS
                           : delay_ms * 2;
}

static void save_pending_credentials_and_connect(void) {
    if (station_ssid_length(&s_pending_config) == 0) {
        ESP_LOGW(TAG, "BLUFI connect request has no SSID");
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR));
        return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
    const esp_err_t error =
        esp_wifi_set_config(WIFI_IF_STA, &s_pending_config);
    if (error != ESP_OK) {
        atomic_store(&s_manual_disconnect, false);
        ESP_LOGE(TAG, "cannot save BLUFI Wi-Fi credentials: %s",
                 esp_err_to_name(error));
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR));
        if (atomic_load(&s_has_credentials)) connect_station();
        return;
    }

    atomic_store(&s_has_credentials, true);
    atomic_store(&s_manual_disconnect, false);
    atomic_store(&s_provisioning, true);
    s_retry_delay_ms = WIFI_RETRY_INITIAL_MS;
    const esp_err_t start_error = ensure_wifi_started();
    if (start_error != ESP_OK) {
        ESP_LOGE(TAG, "cannot start Wi-Fi after BLUFI configuration: %s",
                 esp_err_to_name(start_error));
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_MSG_STATE_ERROR));
        return;
    }
    ESP_LOGI(TAG, "BLUFI credentials saved; connecting to Wi-Fi");
    connect_station();
}

static void apply_pending_credentials(void) {
    if (station_ssid_length(&s_pending_config) == 0) {
        save_pending_credentials_and_connect();
        return;
    }

    atomic_store(&s_manual_disconnect, true);
    stop_retry_timer();

    // esp_wifi_disconnect() can return ESP_OK while the station is already
    // idle, in which case no WIFI_EVENT_STA_DISCONNECTED follows. Apply new
    // credentials immediately instead of waiting forever for that event.
    if (!atomic_load(&s_station_connected) && !atomic_load(&s_connecting)) {
        s_reconfigure_after_disconnect = false;
        save_pending_credentials_and_connect();
        return;
    }

    s_reconfigure_after_disconnect = true;
    const esp_err_t error = esp_wifi_disconnect();
    if (error == ESP_OK) {
        return;
    }
    s_reconfigure_after_disconnect = false;
    if (error != ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGW(TAG, "cannot disconnect before BLUFI reconfiguration: %s",
                 esp_err_to_name(error));
    }
    save_pending_credentials_and_connect();
}

static void reset_provisioning(void) {
    atomic_store(&s_manual_disconnect, true);
    atomic_store(&s_station_connected, false);
    atomic_store(&s_has_ip, false);
    atomic_store(&s_connecting, false);
    atomic_store(&s_provisioning, true);
    stop_retry_timer();

    // A provisioning app starts BLUFI packet sequence numbers from zero for a
    // new attempt. Close an existing GATT connection so the profile resets its
    // sequence counters and the next attempt performs fresh key negotiation.
    if (atomic_load(&s_ble_connected)) {
        ESP_LOGI(TAG, "disconnecting BLUFI client to reset protocol session");
        esp_blufi_disconnect();
    }

    s_reconfigure_after_disconnect = false;
    const esp_err_t disconnect_error = esp_wifi_disconnect();
    if (disconnect_error != ESP_OK &&
        disconnect_error != ESP_ERR_WIFI_NOT_CONNECT &&
        disconnect_error != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGW(TAG, "cannot disconnect while resetting Wi-Fi: %s",
                 esp_err_to_name(disconnect_error));
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t empty_config = {0};
    const esp_err_t clear_error =
        esp_wifi_set_config(WIFI_IF_STA, &empty_config);
    if (clear_error != ESP_OK) {
        ESP_LOGE(TAG, "cannot clear saved Wi-Fi credentials: %s",
                 esp_err_to_name(clear_error));
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_MSG_STATE_ERROR));
    } else {
        atomic_store(&s_has_credentials, false);
        memset(&s_pending_config, 0, sizeof(s_pending_config));
    }

    const esp_err_t start_error = ensure_wifi_started();
    if (start_error != ESP_OK) {
        ESP_LOGE(TAG, "cannot restart Wi-Fi after credential reset: %s",
                 esp_err_to_name(start_error));
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_MSG_STATE_ERROR));
    }
    s_retry_delay_ms = WIFI_RETRY_INITIAL_MS;
    atomic_store(&s_manual_disconnect, clear_error != ESP_OK);

    if (clear_error == ESP_OK && start_error == ESP_OK) {
        ESP_LOGI(TAG,
                 "Wi-Fi credentials cleared; BLUFI provisioning restarted");
    }
    start_advertising();
    report_blufi_status();
}

static void report_scan_results(void) {
    uint16_t count = 0;
    if (esp_wifi_scan_get_ap_num(&count) != ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL));
        return;
    }
    if (count > WIFI_SCAN_RESULT_LIMIT) count = WIFI_SCAN_RESULT_LIMIT;
    if (count == 0) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_blufi_send_wifi_list(0, NULL));
        return;
    }

    wifi_ap_record_t *records = calloc(count, sizeof(*records));
    esp_blufi_ap_record_t *results = calloc(count, sizeof(*results));
    if (!records || !results) {
        free(records);
        free(results);
        esp_wifi_clear_ap_list();
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL));
        return;
    }

    uint16_t returned = count;
    const esp_err_t error = esp_wifi_scan_get_ap_records(&returned, records);
    if (error == ESP_OK) {
        for (uint16_t i = 0; i < returned; ++i) {
            results[i].rssi = records[i].rssi;
            memcpy(results[i].ssid, records[i].ssid,
                   sizeof(records[i].ssid));
        }
        if (atomic_load(&s_ble_connected)) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(
                esp_blufi_send_wifi_list(returned, results));
        }
    } else {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL));
    }
    free(records);
    free(results);
}

static void provisioning_task(void *parameter) {
    (void)parameter;
    provisioning_command_t command;
    for (;;) {
        if (xQueueReceive(s_commands, &command, portMAX_DELAY) != pdTRUE) continue;

        switch (command.type) {
            case COMMAND_WIFI_STARTED:
                if (atomic_load(&s_has_credentials) &&
                    !atomic_load(&s_connecting)) {
                    connect_station();
                } else {
                    atomic_store(&s_provisioning, true);
                    start_advertising();
                }
                break;
            case COMMAND_WIFI_CONNECTED:
                atomic_store(&s_station_connected, true);
                atomic_store(&s_connecting, false);
                report_blufi_status();
                break;
            case COMMAND_WIFI_DISCONNECTED:
                s_last_disconnect_reason = command.value.disconnected.reason;
                s_last_disconnect_rssi = command.value.disconnected.rssi;
                atomic_store(&s_station_connected, false);
                atomic_store(&s_has_ip, false);
                atomic_store(&s_connecting, false);
                report_blufi_status();
                if (s_reconfigure_after_disconnect) {
                    s_reconfigure_after_disconnect = false;
                    save_pending_credentials_and_connect();
                } else {
                    schedule_reconnect();
                }
                break;
            case COMMAND_WIFI_GOT_IP:
                atomic_store(&s_has_ip, true);
                atomic_store(&s_station_connected, true);
                atomic_store(&s_connecting, false);
                atomic_store(&s_provisioning, false);
                s_retry_delay_ms = WIFI_RETRY_INITIAL_MS;
                stop_retry_timer();
                ESP_LOGI(TAG, "Wi-Fi connected, IP=" IPSTR,
                         IP2STR(&command.value.ip));
                report_blufi_status();
                if (!atomic_load(&s_ble_connected)) stop_advertising();
                break;
            case COMMAND_RETRY_CONNECT:
                connect_station();
                break;
            case COMMAND_CONNECT_PENDING:
                apply_pending_credentials();
                break;
            case COMMAND_DISCONNECT:
                atomic_store(&s_manual_disconnect, true);
                atomic_store(&s_provisioning, true);
                stop_retry_timer();
                const esp_err_t disconnect_error = esp_wifi_disconnect();
                if (disconnect_error == ESP_ERR_WIFI_NOT_CONNECT ||
                    disconnect_error == ESP_ERR_WIFI_NOT_STARTED) {
                    report_blufi_status();
                } else if (disconnect_error != ESP_OK) {
                    ESP_LOGW(TAG, "BLUFI Wi-Fi disconnect failed: %s",
                             esp_err_to_name(disconnect_error));
                }
                break;
            case COMMAND_SET_SSID:
                memset(s_pending_config.sta.ssid, 0,
                       sizeof(s_pending_config.sta.ssid));
                memcpy(s_pending_config.sta.ssid, command.value.data.bytes,
                       command.value.data.length);
                memset(s_pending_config.sta.password, 0,
                       sizeof(s_pending_config.sta.password));
                s_pending_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
                s_pending_config.sta.bssid_set = false;
                memset(s_pending_config.sta.bssid, 0,
                       sizeof(s_pending_config.sta.bssid));
                ESP_LOGI(TAG, "BLUFI received station SSID (%u bytes)",
                         command.value.data.length);
                break;
            case COMMAND_SET_PASSWORD:
                memset(s_pending_config.sta.password, 0,
                       sizeof(s_pending_config.sta.password));
                memcpy(s_pending_config.sta.password, command.value.data.bytes,
                       command.value.data.length);
                s_pending_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
                ESP_LOGI(TAG, "BLUFI received station password (%u bytes)",
                         command.value.data.length);
                break;
            case COMMAND_SET_BSSID:
                memcpy(s_pending_config.sta.bssid, command.value.data.bytes,
                       sizeof(s_pending_config.sta.bssid));
                s_pending_config.sta.bssid_set = true;
                break;
            case COMMAND_START_ADVERTISING:
                start_advertising();
                break;
            case COMMAND_REPORT_STATUS:
                report_blufi_status();
                break;
            case COMMAND_START_SCAN: {
                const wifi_scan_config_t scan = {
                    .ssid = NULL,
                    .bssid = NULL,
                    .channel = 0,
                    .show_hidden = false,
                };
                esp_err_t error = ensure_wifi_started();
                if (error == ESP_OK) {
                    error = esp_wifi_scan_start(&scan, false);
                }
                if (error != ESP_OK) {
                    ESP_LOGW(TAG, "Wi-Fi scan failed to start: %s",
                             esp_err_to_name(error));
                    ESP_ERROR_CHECK_WITHOUT_ABORT(
                        esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL));
                }
                break;
            }
            case COMMAND_REPORT_SCAN:
                report_scan_results();
                break;
            case COMMAND_RESET_PROVISIONING:
                reset_provisioning();
                break;
        }
    }
}

static void retry_timer_callback(void *parameter) {
    (void)parameter;
    post_simple_command(COMMAND_RETRY_CONNECT);
}

static void wifi_event_handler(void *argument, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    (void)argument;
    (void)event_base;
    provisioning_command_t command = {0};

    switch (event_id) {
        case WIFI_EVENT_STA_START:
            atomic_store(&s_wifi_started, true);
            command.type = COMMAND_WIFI_STARTED;
            break;
        case WIFI_EVENT_STA_STOP:
            atomic_store(&s_wifi_started, false);
            atomic_store(&s_station_connected, false);
            atomic_store(&s_has_ip, false);
            atomic_store(&s_connecting, false);
            return;
        case WIFI_EVENT_STA_CONNECTED:
            command.type = COMMAND_WIFI_CONNECTED;
            break;
        case WIFI_EVENT_STA_DISCONNECTED: {
            const wifi_event_sta_disconnected_t *event = event_data;
            command.type = COMMAND_WIFI_DISCONNECTED;
            command.value.disconnected.reason = event->reason;
            command.value.disconnected.rssi = event->rssi;
            break;
        }
        case WIFI_EVENT_SCAN_DONE:
            command.type = COMMAND_REPORT_SCAN;
            break;
        default:
            return;
    }
    post_command(&command);
}

static void ip_event_handler(void *argument, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
    (void)argument;
    (void)event_base;
    if (event_id != IP_EVENT_STA_GOT_IP) return;
    const ip_event_got_ip_t *event = event_data;
    provisioning_command_t command = {.type = COMMAND_WIFI_GOT_IP};
    command.value.ip = event->ip_info.ip;
    post_command(&command);
}

static bool post_received_data(provisioning_command_type_t type,
                               const uint8_t *data, int length,
                               size_t maximum_length, bool allow_empty) {
    if ((!data && length != 0) || length < 0 ||
        (length == 0 && !allow_empty) ||
        (size_t)length > maximum_length) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR));
        return false;
    }
    provisioning_command_t command = {.type = type};
    command.value.data.length = length;
    if (length != 0) memcpy(command.value.data.bytes, data, length);
    if (!post_command(&command)) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_blufi_send_error_info(ESP_BLUFI_MSG_STATE_ERROR));
        return false;
    }
    return true;
}

static void blufi_event_handler(esp_blufi_cb_event_t event,
                                esp_blufi_cb_param_t *parameter) {
    switch (event) {
        case ESP_BLUFI_EVENT_INIT_FINISH:
            atomic_store(&s_blufi_ready, true);
            post_simple_command(COMMAND_START_ADVERTISING);
            break;
        case ESP_BLUFI_EVENT_DEINIT_FINISH:
            atomic_store(&s_blufi_ready, false);
            atomic_store(&s_advertising, false);
            break;
        case ESP_BLUFI_EVENT_BLE_CONNECT:
            atomic_store(&s_ble_connected, true);
            atomic_store(&s_advertising, false);
            esp_blufi_adv_stop();
            if (demo_blufi_security_init() != ESP_OK) {
                ESP_ERROR_CHECK_WITHOUT_ABORT(
                    esp_blufi_send_error_info(ESP_BLUFI_INIT_SECURITY_ERROR));
            }
            ESP_LOGI(TAG, "BLUFI client connected");
            break;
        case ESP_BLUFI_EVENT_BLE_DISCONNECT:
            atomic_store(&s_ble_connected, false);
            demo_blufi_security_deinit();
            ESP_LOGI(TAG, "BLUFI client disconnected");
            post_simple_command(COMMAND_START_ADVERTISING);
            break;
        case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
            if (!parameter || parameter->wifi_mode.op_mode != WIFI_MODE_STA) {
                ESP_ERROR_CHECK_WITHOUT_ABORT(
                    esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR));
            }
            break;
        case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
            post_simple_command(COMMAND_CONNECT_PENDING);
            break;
        case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
            post_simple_command(COMMAND_DISCONNECT);
            break;
        case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
            post_simple_command(COMMAND_REPORT_STATUS);
            break;
        case ESP_BLUFI_EVENT_RECV_STA_SSID:
            if (parameter) {
                post_received_data(COMMAND_SET_SSID, parameter->sta_ssid.ssid,
                                   parameter->sta_ssid.ssid_len,
                                   sizeof(s_pending_config.sta.ssid), false);
            }
            break;
        case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
            if (parameter) {
                post_received_data(COMMAND_SET_PASSWORD,
                                   parameter->sta_passwd.passwd,
                                   parameter->sta_passwd.passwd_len,
                                   sizeof(s_pending_config.sta.password), true);
            }
            break;
        case ESP_BLUFI_EVENT_RECV_STA_BSSID:
            if (parameter) {
                post_received_data(COMMAND_SET_BSSID,
                                   parameter->sta_bssid.bssid,
                                   sizeof(parameter->sta_bssid.bssid),
                                   sizeof(s_pending_config.sta.bssid), false);
            }
            break;
        case ESP_BLUFI_EVENT_GET_WIFI_LIST:
            post_simple_command(COMMAND_START_SCAN);
            break;
        case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
            esp_blufi_disconnect();
            break;
        case ESP_BLUFI_EVENT_REPORT_ERROR:
            if (parameter) {
                ESP_LOGE(TAG, "BLUFI protocol error: %d",
                         parameter->report_error.state);
            }
            break;
        default:
            break;
    }
}

static esp_err_t initialize_nvs(void) {
    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES ||
        error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "cannot erase invalid NVS");
        error = nvs_flash_init();
    }
    return error;
}

static esp_err_t initialize_bluetooth(void) {
    esp_bt_controller_config_t controller_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_bt_controller_init(&controller_config), TAG,
                        "cannot initialize BLE controller");
    ESP_RETURN_ON_ERROR(esp_bt_controller_enable(ESP_BT_MODE_BLE), TAG,
                        "cannot enable BLE controller");
    ESP_RETURN_ON_ERROR(esp_bluedroid_init(), TAG,
                        "cannot initialize Bluedroid");
    ESP_RETURN_ON_ERROR(esp_bluedroid_enable(), TAG,
                        "cannot enable Bluedroid");
    ESP_RETURN_ON_ERROR(esp_ble_gatt_set_local_mtu(BLUFI_GATT_LOCAL_MTU), TAG,
                        "cannot limit BLUFI GATT MTU");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_device_name(s_device_name), TAG,
                        "cannot set BLUFI device name");
    ESP_RETURN_ON_ERROR(esp_blufi_register_callbacks(&s_blufi_callbacks), TAG,
                        "cannot register BLUFI callbacks");
    ESP_RETURN_ON_ERROR(esp_ble_gap_register_callback(esp_blufi_gap_event_handler),
                        TAG, "cannot register BLE GAP callback");
    return esp_blufi_profile_init();
}

esp_err_t demo_blufi_init(void) {
    if (atomic_load(&s_initialized)) return ESP_ERR_INVALID_STATE;

    ESP_RETURN_ON_ERROR(initialize_nvs(), TAG, "cannot initialize NVS");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "cannot initialize TCP/IP stack");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG,
                        "cannot create default event loop");
    if (!esp_netif_create_default_wifi_sta()) return ESP_ERR_NO_MEM;

    s_commands = xQueueCreate(PROVISIONING_QUEUE_LENGTH,
                              sizeof(provisioning_command_t));
    if (!s_commands) return ESP_ERR_NO_MEM;
    if (xTaskCreate(provisioning_task, "blufi_demo", PROVISIONING_TASK_STACK_SIZE,
                    NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    const esp_timer_create_args_t retry_timer_config = {
        .callback = retry_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_retry",
        .skip_unhandled_events = true,
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&retry_timer_config, &s_retry_timer),
                        TAG, "cannot create reconnect timer");

    ESP_RETURN_ON_ERROR(esp_event_handler_register(
                            WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL),
                        TAG, "cannot register Wi-Fi event handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(
                            IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handler, NULL),
                        TAG, "cannot register IP event handler");

    const wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG,
                        "cannot initialize Wi-Fi");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_FLASH), TAG,
                        "cannot enable saved Wi-Fi configuration");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG,
                        "cannot enable station mode");
    ESP_RETURN_ON_ERROR(esp_wifi_get_config(WIFI_IF_STA, &s_pending_config), TAG,
                        "cannot read saved Wi-Fi configuration");
    atomic_store(&s_has_credentials,
                 station_ssid_length(&s_pending_config) != 0);

    uint8_t mac[6] = {0};
    ESP_RETURN_ON_ERROR(esp_read_mac(mac, ESP_MAC_BT), TAG,
                        "cannot read BLE address");
    snprintf(s_device_name, sizeof(s_device_name), "SunflowerBuddy-%02X%02X",
             mac[4], mac[5]);

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "cannot start Wi-Fi");
    ESP_RETURN_ON_ERROR(initialize_bluetooth(), TAG, "cannot initialize BLUFI");
    atomic_store(&s_initialized, true);

    ESP_LOGI(TAG, "BLUFI %04x initialized; saved Wi-Fi credentials: %s",
             esp_blufi_get_version(),
             atomic_load(&s_has_credentials) ? "yes" : "no");
    return ESP_OK;
}

esp_err_t demo_blufi_reset_provisioning(void) {
    if (!atomic_load(&s_initialized) || !s_commands) {
        return ESP_ERR_INVALID_STATE;
    }
    const provisioning_command_t command = {
        .type = COMMAND_RESET_PROVISIONING,
    };
    return post_command(&command) ? ESP_OK : ESP_ERR_TIMEOUT;
}

void demo_blufi_log_status(void) {
    ESP_LOGI(TAG,
             "Wi-Fi credentials=%s connected=%s ip=%s; BLUFI provisioning=%s "
             "client=%s",
             atomic_load(&s_has_credentials) ? "saved" : "none",
             atomic_load(&s_station_connected) ? "yes" : "no",
             atomic_load(&s_has_ip) ? "ready" : "none",
             atomic_load(&s_provisioning) ? "active" : "idle",
             atomic_load(&s_ble_connected) ? "connected" : "none");
}
