#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Starts station Wi-Fi, restores saved credentials, and initializes BLUFI.
esp_err_t demo_blufi_init(void);

// Queues a reset of saved station credentials and re-enters BLUFI provisioning.
esp_err_t demo_blufi_reset_provisioning(void);

// Logs the current station/provisioning state without exposing credentials.
void demo_blufi_log_status(void);

#ifdef __cplusplus
}
#endif
