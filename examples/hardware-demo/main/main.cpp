// Sunflower Buddy V1.4 BSP reference demo. Product state machines and cloud services do not belong here.
#include "demo.hpp"
#include "demo_blufi.h"
#include "bsp_audio.h"
#include "bsp_button.h"
#include "bsp_i2c.h"
#include "bsp_led.h"
#include "bsp_motor.hpp"
#include "bsp_pins.h"
#include "bsp_power.hpp"
#include "bsp_rtc.hpp"
#include "bsp_wakeup.hpp"

#include "esp_log.h"
#include "esp_littlefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "main";
static QueueHandle_t s_events;
static uint8_t s_volume = 60;

static void on_power(uint32_t events, void *) {
    ESP_LOGI(TAG, "AXP2101 event mask: 0x%08lx", static_cast<unsigned long>(events));
}

static void on_rtc_alarm(void *) { ESP_LOGI(TAG, "BM8563 alarm interrupt"); }

static void on_motor_overcurrent(int raw, int filtered, void *) {
    ESP_LOGW(TAG, "motor overcurrent: raw=%d filtered=%d; motor stopped", raw, filtered);
}

static void mount_firmware_storage() {
    esp_vfs_littlefs_conf_t config = {};
    config.base_path = "/littlefs";
    config.partition_label = "storage";
    config.format_if_mount_failed = false;
    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&config));
    ESP_LOGI(TAG, "GX8002 firmware available under /littlefs/gx_bl");
}

enum class demo_event_type_t {
    BUTTON,
    WAKE_WORD,
};

struct demo_event_t {
    demo_event_type_t type;
    bsp_button_t button;
    bsp_button_event_t event;
};

static void on_button(bsp_button_t button, bsp_button_event_t event, void *) {
    const demo_event_t message{demo_event_type_t::BUTTON, button, event};
    xQueueSend(s_events, &message, 0);
}

static void on_wake_word(void *) {
    const demo_event_t message{
        demo_event_type_t::WAKE_WORD, BSP_BUTTON_COUNT, BSP_BUTTON_PRESSED};
    xQueueSend(s_events, &message, 0);
}

static void demo_task(void *) {
    demo_event_t message{};
    for (;;) {
        if (xQueueReceive(s_events, &message, portMAX_DELAY) != pdTRUE) continue;

        if (message.type == demo_event_type_t::WAKE_WORD) {
            demo_run_wakeup_feedback();
        } else if (message.button == BSP_BUTTON_VOICE &&
                   message.event == BSP_BUTTON_PRESSED) {
            demo_record_and_playback();
        } else if (message.button == BSP_BUTTON_WIFI &&
                   message.event == BSP_BUTTON_LONG_PRESSED) {
            ESP_LOGI(TAG, "Wi-Fi button held: resetting BLUFI provisioning");
            ESP_ERROR_CHECK_WITHOUT_ABORT(demo_blufi_reset_provisioning());
        } else if (message.event != BSP_BUTTON_CLICKED) {
            continue;
        } else if (message.button == BSP_BUTTON_WIFI) {
            demo_blufi_log_status();
            demo_run_self_test();
        } else if (message.button == BSP_BUTTON_VOLUME_UP) {
            const uint8_t next = s_volume <= 90 ? s_volume + 10 : 100;
            if (bsp_audio_set_volume(next) == ESP_OK) s_volume = next;
            ESP_LOGI(TAG, "playback volume: %u%%", s_volume);
        } else if (message.button == BSP_BUTTON_VOLUME_DOWN) {
            const uint8_t next = s_volume >= 10 ? s_volume - 10 : 0;
            if (bsp_audio_set_volume(next) == ESP_OK) s_volume = next;
            ESP_LOGI(TAG, "playback volume: %u%%", s_volume);
        }
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Sunflower Buddy V1.4 BSP demo");
    mount_firmware_storage();
    ESP_ERROR_CHECK(demo_blufi_init());
    ESP_ERROR_CHECK(bsp_i2c_init());
    bsp_i2c_scan();

    ESP_ERROR_CHECK(bsp_power_init());
    ESP_ERROR_CHECK(bsp_power_set_callback(on_power, nullptr));
    ESP_LOGI(TAG, "RTC: %s", esp_err_to_name(bsp_rtc_init()));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_rtc_set_callback(on_rtc_alarm, nullptr));
    ESP_LOGI(TAG, "led: %s", esp_err_to_name(bsp_led_init()));
    ESP_LOGI(TAG, "motor: %s", esp_err_to_name(bsp_motor_init()));
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        bsp_motor_enable_overcurrent_monitor(2500, 100, on_motor_overcurrent, nullptr));
    ESP_ERROR_CHECK(bsp_audio_init());
    ESP_ERROR_CHECK(bsp_audio_open(BSP_AUDIO_RATE, 16, 1));
    ESP_ERROR_CHECK(bsp_audio_set_volume(s_volume));
    ESP_LOGI(TAG, "audio ready: %u Hz, mono, initial volume %u%%", BSP_AUDIO_RATE, s_volume);
    ESP_LOGI(TAG, "GX8002: %s", esp_err_to_name(bsp_wakeup_init()));

    s_events = xQueueCreate(8, sizeof(demo_event_t));
    ESP_ERROR_CHECK(s_events ? ESP_OK : ESP_ERR_NO_MEM);
    bsp_wakeup_set_callback(on_wake_word, nullptr);
    ESP_ERROR_CHECK(bsp_button_init(on_button, nullptr));
    xTaskCreate(demo_task, "demo", 4096, nullptr, 4, nullptr);

    ESP_LOGI(TAG,
             "Hold voice to record; hold Wi-Fi to reset BLUFI; GX8002 wake word drives motor");
    demo_print_status();
}
