#include "bsp_motor.hpp"
#include "bsp_pins.h"
#include "port_mx6115.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <memory>
#include <new>

static std::unique_ptr<MX6115> s_motor;
static TaskHandle_t s_ocp_task;

static MX6115::Mode to_driver_mode(bsp_motor_mode_t mode) {
    return static_cast<MX6115::Mode>(static_cast<int>(mode));
}

esp_err_t bsp_motor_init() {
    if (s_motor) return ESP_OK;
    const MX6115::pwm_config_t config = {
        .ina_gpio = BSP_MOTOR_INA, .inb_gpio = BSP_MOTOR_INB, .pwm_freq = 20000,
        .pwm_res = LEDC_TIMER_10_BIT, .pwm_timer = LEDC_TIMER_0,
        .pwm_mode = LEDC_LOW_SPEED_MODE, .ina_ch = LEDC_CHANNEL_0, .inb_ch = LEDC_CHANNEL_1,
    };
    s_motor = std::make_unique<MX6115>(config, MX6115::Mode::STOP, 200, 0, 700);
    return ESP_OK;
}

esp_err_t bsp_motor_start(uint32_t duty) {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    if (duty > 1023) return ESP_ERR_INVALID_ARG;
    s_motor->setMode(MX6115::Mode::FORWARD);
    s_motor->setSpeed(duty);
    // The upstream driver returns the FreeRTOS queue result despite declaring
    // esp_err_t: pdPASS (1) means success, not ESP_FAIL.
    return s_motor->start() == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t bsp_motor_pulse(uint32_t duration_ms, uint32_t duty) {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    if (duty > 1023) return ESP_ERR_INVALID_ARG;
    return s_motor->startOnce(MX6115::Mode::FORWARD, duration_ms, 0, duty);
}

esp_err_t bsp_motor_stop() {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    return s_motor->stop() == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t bsp_motor_set_mode(bsp_motor_mode_t mode) {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    s_motor->setMode(to_driver_mode(mode));
    return ESP_OK;
}

esp_err_t bsp_motor_set_speed(uint32_t duty) {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    if (duty > 1023) return ESP_ERR_INVALID_ARG;
    s_motor->setSpeed(duty);
    return ESP_OK;
}

esp_err_t bsp_motor_set_stop_duration(uint32_t duration_ms) {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    s_motor->setStopDurationMs(duration_ms);
    return ESP_OK;
}

struct OcpConfig {
    int threshold;
    uint32_t trigger_ms;
    bsp_motor_ocp_callback_t callback;
    void *user_data;
};

static void ocp_task(void *argument) {
    std::unique_ptr<OcpConfig> config(static_cast<OcpConfig *>(argument));
    adc_oneshot_unit_handle_t adc = nullptr;
    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_2, .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_new_unit(&unit_config, &adc) != ESP_OK ||
        adc_oneshot_config_channel(adc, ADC_CHANNEL_3, &channel_config) != ESP_OK) {
        ESP_LOGE("bsp_motor", "cannot initialize GPIO14 overcurrent ADC");
        s_ocp_task = nullptr;
        vTaskDelete(nullptr);
    }
    int filtered = 0;
    uint32_t over_ms = 0;
    bool reported = false;
    for (;;) {
        int raw = 0;
        if (adc_oneshot_read(adc, ADC_CHANNEL_3, &raw) == ESP_OK) {
            filtered = filtered == 0 ? raw : (filtered * 9 + raw) / 10;
            if (filtered > config->threshold) {
                over_ms += 20;
                if (!reported && over_ms >= config->trigger_ms) {
                    bsp_motor_stop();
                    if (config->callback) config->callback(raw, filtered, config->user_data);
                    reported = true;
                }
            } else if (filtered < config->threshold - 200) {
                over_ms = 0;
                reported = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t bsp_motor_enable_overcurrent_monitor(int threshold, uint32_t trigger_ms,
                                               bsp_motor_ocp_callback_t callback,
                                               void *user_data) {
    if (!s_motor) return ESP_ERR_INVALID_STATE;
    if (s_ocp_task) return ESP_OK;
    if (threshold <= 0 || trigger_ms == 0) return ESP_ERR_INVALID_ARG;
    auto *config = new (std::nothrow) OcpConfig{threshold, trigger_ms, callback, user_data};
    if (!config) return ESP_ERR_NO_MEM;
    if (xTaskCreatePinnedToCore(ocp_task, "motor_ocp", 3072, config, 6,
                                &s_ocp_task, 1) != pdPASS) {
        delete config;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
