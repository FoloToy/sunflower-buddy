// Sunflower Buddy V1.4 hardware facts. Application code must not duplicate these values.
#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"
#include "driver/i2s_types.h"
#include "driver/uart.h"

#define BSP_I2C_PORT I2C_NUM_0
#define BSP_I2C_SDA  GPIO_NUM_41
#define BSP_I2C_SCL  GPIO_NUM_42
#define BSP_I2C_HZ   400000

#define BSP_I2C_AXP2101_ADDR 0x34
#define BSP_I2C_BM8563_ADDR  0x51
#define BSP_I2C_GX8002_ADDR  0x2F
#define BSP_I2C_ES8311_ADDR  0x18

#define BSP_PMU_IRQ    GPIO_NUM_2
#define BSP_RTC_IRQ    GPIO_NUM_1
#define BSP_GX8002_IRQ GPIO_NUM_13

#define BSP_LED_DATA GPIO_NUM_6

#define BSP_BUTTON_WIFI_GPIO        GPIO_NUM_21
#define BSP_BUTTON_VOICE_GPIO       GPIO_NUM_12
#define BSP_BUTTON_VOLUME_UP_GPIO   GPIO_NUM_7
#define BSP_BUTTON_VOLUME_DOWN_GPIO GPIO_NUM_16

#define BSP_MOTOR_INA GPIO_NUM_47
#define BSP_MOTOR_INB GPIO_NUM_38
#define BSP_MOTOR_OCP GPIO_NUM_14

#define BSP_I2S_PORT   I2S_NUM_0
#define BSP_I2S_MCLK   GPIO_NUM_11
#define BSP_I2S_BCLK   GPIO_NUM_10
#define BSP_I2S_WS     GPIO_NUM_46
// ESP32-S3 signal direction: TX goes to ES8311 DIN; RX comes from ES8311 DOUT.
#define BSP_I2S_DOUT   GPIO_NUM_3
#define BSP_I2S_DIN    GPIO_NUM_9
#define BSP_AUDIO_PA   GPIO_NUM_15
#define BSP_AUDIO_RATE 24000

// The labels describe ESP32-S3 signals. The PCB crosses them to the modem.
#define BSP_MODEM_UART         UART_NUM_0
#define BSP_MODEM_TX           GPIO_NUM_5
#define BSP_MODEM_RX           GPIO_NUM_4
#define BSP_MODEM_INITIAL_BAUD 115200
#define BSP_MODEM_TARGET_BAUD  921600

// Board configuration consumed by the FoloToy AXP2101 driver.
#define BSP_PMU_TS_TYPE           1
#define BSP_PMU_BATTERY_TYPE      2
#define BSP_PMU_SLEEP_TYPE        1
#define BSP_PMU_POWER_SWITCH_TYPE 1
#define BSP_PMU_BOARD_TYPE ((BSP_PMU_TS_TYPE << 0) | \
                            (BSP_PMU_BATTERY_TYPE << 4) | \
                            (BSP_PMU_SLEEP_TYPE << 8) | \
                            (BSP_PMU_POWER_SWITCH_TYPE << 12))
