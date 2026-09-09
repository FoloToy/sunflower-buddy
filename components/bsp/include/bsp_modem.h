#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t bsp_modem_uart_init(uint32_t baud_rate);
esp_err_t bsp_modem_uart_set_baud(uint32_t baud_rate);
int bsp_modem_uart_write(const void *data, size_t size);
int bsp_modem_uart_read(void *data, size_t size, uint32_t timeout_ms);
esp_err_t bsp_modem_uart_flush(void);

#ifdef __cplusplus
}
#endif
