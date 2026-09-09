#include "bsp_modem.h"
#include "bsp_pins.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

esp_err_t bsp_modem_uart_init(uint32_t baud_rate) {
    if (baud_rate == 0) return ESP_ERR_INVALID_ARG;
    const uart_config_t config = {
        .baud_rate = static_cast<int>(baud_rate), .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT, .flags = {},
    };
    esp_err_t err = uart_param_config(BSP_MODEM_UART, &config);
    if (err != ESP_OK) return err;
    err = uart_set_pin(BSP_MODEM_UART, BSP_MODEM_TX, BSP_MODEM_RX,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;
    if (!uart_is_driver_installed(BSP_MODEM_UART))
        return uart_driver_install(BSP_MODEM_UART, 4096, 4096, 0, nullptr, 0);
    return ESP_OK;
}

esp_err_t bsp_modem_uart_set_baud(uint32_t baud_rate) {
    if (baud_rate == 0) return ESP_ERR_INVALID_ARG;
    return uart_set_baudrate(BSP_MODEM_UART, baud_rate);
}

int bsp_modem_uart_write(const void *data, size_t size) {
    if (!data && size) return -1;
    return uart_write_bytes(BSP_MODEM_UART, data, size);
}

int bsp_modem_uart_read(void *data, size_t size, uint32_t timeout_ms) {
    if (!data && size) return -1;
    return uart_read_bytes(BSP_MODEM_UART, data, size, pdMS_TO_TICKS(timeout_ms));
}

esp_err_t bsp_modem_uart_flush(void) { return uart_flush(BSP_MODEM_UART); }
