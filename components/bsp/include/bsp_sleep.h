#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t bsp_sleep_default_wakeup_mask(void);
esp_err_t bsp_sleep_configure_wakeup(uint64_t gpio_mask);
// Closes audio, puts AXP2101 into its board sleep state, then enters ESP deep sleep.
esp_err_t bsp_sleep_enter(uint64_t gpio_mask);

#ifdef __cplusplus
}
#endif
