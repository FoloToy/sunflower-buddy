/*
 * Adapted from the ESP-IDF 5.5.3 BLUFI example.
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "demo_blufi_security.h"

#include "esp_blufi_api.h"
#include "esp_crc.h"
#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"
#include "mbedtls/md5.h"

#include <stdlib.h>
#include <string.h>

enum {
    SEC_TYPE_DH_PARAM_LEN = 0x00,
    SEC_TYPE_DH_PARAM_DATA = 0x01,
    SEC_TYPE_DH_P = 0x02,
    SEC_TYPE_DH_G = 0x03,
    SEC_TYPE_DH_PUBLIC = 0x04,
};

enum {
    DH_SELF_PUB_KEY_LEN = 128,
    DH_PARAM_LEN_MAX = 1024,
    SHARE_KEY_LEN = 128,
    PSK_LEN = 16,
};

typedef struct {
    uint8_t self_public_key[DH_SELF_PUB_KEY_LEN];
    uint8_t share_key[SHARE_KEY_LEN];
    size_t share_len;
    uint8_t psk[PSK_LEN];
    uint8_t *dh_param;
    int dh_param_len;
    uint8_t iv[16];
    mbedtls_dhm_context dhm;
    mbedtls_aes_context aes;
} blufi_security_t;

static const char *TAG = "blufi_security";
static blufi_security_t *s_security;

static int fill_random(void *state, unsigned char *output, size_t len) {
    (void)state;
    esp_fill_random(output, len);
    return 0;
}

static void report_error(esp_blufi_error_state_t error) {
    ESP_LOGE(TAG, "security negotiation failed: %d", error);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_blufi_send_error_info(error));
}

void demo_blufi_negotiate_data(uint8_t *data, int len, uint8_t **output_data,
                               int *output_len, bool *need_free) {
    if (output_data) *output_data = NULL;
    if (output_len) *output_len = 0;
    if (need_free) *need_free = false;
    if (!data || len < 3 || !output_data || !output_len || !need_free) {
        report_error(ESP_BLUFI_DATA_FORMAT_ERROR);
        return;
    }
    if (!s_security) {
        report_error(ESP_BLUFI_INIT_SECURITY_ERROR);
        return;
    }

    const uint8_t type = data[0];
    if (type == SEC_TYPE_DH_PARAM_LEN) {
        const int parameter_len = (data[1] << 8) | data[2];
        if (parameter_len <= 0 || parameter_len > DH_PARAM_LEN_MAX) {
            report_error(ESP_BLUFI_DH_PARAM_ERROR);
            return;
        }
        free(s_security->dh_param);
        s_security->dh_param = malloc(parameter_len);
        if (!s_security->dh_param) {
            s_security->dh_param_len = 0;
            report_error(ESP_BLUFI_DH_MALLOC_ERROR);
            return;
        }
        s_security->dh_param_len = parameter_len;
        return;
    }

    if (type == SEC_TYPE_DH_PARAM_DATA) {
        if (!s_security->dh_param || s_security->dh_param_len <= 0 ||
            len < s_security->dh_param_len + 1) {
            report_error(ESP_BLUFI_DH_PARAM_ERROR);
            return;
        }

        memcpy(s_security->dh_param, data + 1, s_security->dh_param_len);
        uint8_t *parameter = s_security->dh_param;
        int result = mbedtls_dhm_read_params(
            &s_security->dhm, &parameter,
            s_security->dh_param + s_security->dh_param_len);
        free(s_security->dh_param);
        s_security->dh_param = NULL;
        s_security->dh_param_len = 0;
        if (result != 0) {
            report_error(ESP_BLUFI_READ_PARAM_ERROR);
            return;
        }

        const int dhm_len = mbedtls_dhm_get_len(&s_security->dhm);
        if (dhm_len <= 0 || dhm_len > DH_SELF_PUB_KEY_LEN) {
            report_error(ESP_BLUFI_DH_PARAM_ERROR);
            return;
        }
        result = mbedtls_dhm_make_public(
            &s_security->dhm, dhm_len, s_security->self_public_key,
            sizeof(s_security->self_public_key), fill_random, NULL);
        if (result != 0) {
            report_error(ESP_BLUFI_MAKE_PUBLIC_ERROR);
            return;
        }
        result = mbedtls_dhm_calc_secret(
            &s_security->dhm, s_security->share_key,
            sizeof(s_security->share_key), &s_security->share_len,
            fill_random, NULL);
        if (result != 0) {
            report_error(ESP_BLUFI_DH_PARAM_ERROR);
            return;
        }
        result = mbedtls_md5(s_security->share_key, s_security->share_len,
                             s_security->psk);
        if (result != 0) {
            report_error(ESP_BLUFI_CALC_MD5_ERROR);
            return;
        }
        result = mbedtls_aes_setkey_enc(&s_security->aes, s_security->psk,
                                        PSK_LEN * 8);
        if (result != 0) {
            report_error(ESP_BLUFI_ENCRYPT_ERROR);
            return;
        }

        *output_data = s_security->self_public_key;
        *output_len = dhm_len;
        return;
    }

    if (type != SEC_TYPE_DH_P && type != SEC_TYPE_DH_G &&
        type != SEC_TYPE_DH_PUBLIC) {
        report_error(ESP_BLUFI_DATA_FORMAT_ERROR);
    }
}

static int transform_data(int mode, uint8_t iv8, uint8_t *data, int data_len) {
    if (!s_security || !data || data_len < 0) return -1;

    size_t iv_offset = 0;
    uint8_t iv[sizeof(s_security->iv)];
    memcpy(iv, s_security->iv, sizeof(iv));
    iv[0] = iv8;
    const int result = mbedtls_aes_crypt_cfb128(
        &s_security->aes, mode, data_len, &iv_offset, iv, data, data);
    return result == 0 ? data_len : -1;
}

int demo_blufi_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len) {
    return transform_data(MBEDTLS_AES_ENCRYPT, iv8, crypt_data, crypt_len);
}

int demo_blufi_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len) {
    return transform_data(MBEDTLS_AES_DECRYPT, iv8, crypt_data, crypt_len);
}

uint16_t demo_blufi_checksum(uint8_t iv8, uint8_t *data, int len) {
    (void)iv8;
    return esp_crc16_be(0, data, len);
}

esp_err_t demo_blufi_security_init(void) {
    demo_blufi_security_deinit();
    s_security = calloc(1, sizeof(*s_security));
    if (!s_security) return ESP_ERR_NO_MEM;
    mbedtls_dhm_init(&s_security->dhm);
    mbedtls_aes_init(&s_security->aes);
    return ESP_OK;
}

void demo_blufi_security_deinit(void) {
    if (!s_security) return;
    free(s_security->dh_param);
    mbedtls_dhm_free(&s_security->dhm);
    mbedtls_aes_free(&s_security->aes);
    memset(s_security, 0, sizeof(*s_security));
    free(s_security);
    s_security = NULL;
}
