#include "bsp_sleep.h"
#include "bsp_audio.h"
#include "bsp_pins.h"
#include "bsp_power.hpp"
#include "esp_sleep.h"

uint64_t bsp_sleep_default_wakeup_mask(void) {
    return (1ULL << BSP_BUTTON_WIFI_GPIO) | (1ULL << BSP_BUTTON_VOICE_GPIO) |
           (1ULL << BSP_BUTTON_VOLUME_UP_GPIO) | (1ULL << BSP_BUTTON_VOLUME_DOWN_GPIO) |
           (1ULL << BSP_PMU_IRQ) | (1ULL << BSP_GX8002_IRQ) |
           (1ULL << BSP_RTC_IRQ);
}

esp_err_t bsp_sleep_configure_wakeup(uint64_t gpio_mask) {
    if (gpio_mask == 0) return ESP_ERR_INVALID_ARG;
    return esp_sleep_enable_ext1_wakeup_io(gpio_mask, ESP_EXT1_WAKEUP_ANY_LOW);
}

esp_err_t bsp_sleep_enter(uint64_t gpio_mask) {
    esp_err_t err = bsp_sleep_configure_wakeup(gpio_mask);
    if (err != ESP_OK) return err;
    err = bsp_audio_close();
    if (err != ESP_OK) return err;
    err = bsp_power_enter_sleep();
    if (err != ESP_OK) return err;
    esp_deep_sleep_start();
    return ESP_OK;
}
