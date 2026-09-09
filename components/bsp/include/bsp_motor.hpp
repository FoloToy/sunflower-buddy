#pragma once

#include "esp_err.h"
#include <cstdint>

enum class bsp_motor_mode_t { STOP, FORWARD, REVERSE, FORWARD_THEN_REVERSE };
using bsp_motor_ocp_callback_t = void (*)(int raw, int filtered, void *user_data);

esp_err_t bsp_motor_init();
esp_err_t bsp_motor_start(uint32_t duty = 1023);
esp_err_t bsp_motor_pulse(uint32_t duration_ms, uint32_t duty = 700);
esp_err_t bsp_motor_stop();
esp_err_t bsp_motor_set_mode(bsp_motor_mode_t mode);
esp_err_t bsp_motor_set_speed(uint32_t duty);
esp_err_t bsp_motor_set_stop_duration(uint32_t duration_ms);
esp_err_t bsp_motor_enable_overcurrent_monitor(int threshold = 2500,
                                               uint32_t trigger_ms = 100,
                                               bsp_motor_ocp_callback_t callback = nullptr,
                                               void *user_data = nullptr);
