#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void demo_blufi_negotiate_data(uint8_t *data, int len, uint8_t **output_data,
                               int *output_len, bool *need_free);
int demo_blufi_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
int demo_blufi_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
uint16_t demo_blufi_checksum(uint8_t iv8, uint8_t *data, int len);
esp_err_t demo_blufi_security_init(void);
void demo_blufi_security_deinit(void);

#ifdef __cplusplus
}
#endif
