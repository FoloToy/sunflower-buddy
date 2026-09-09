#include "demo.hpp"

#include "bsp_audio.h"
#include "bsp_button.h"
#include "bsp_led.h"
#include "bsp_motor.hpp"
#include "bsp_power.hpp"
#include "bsp_wakeup.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"

#include <algorithm>
#include <cstdint>
#include <memory>

namespace {

constexpr uint32_t kSampleRate = 24000;
constexpr uint8_t kBitsPerSample = 16;
constexpr uint8_t kChannels = 1;
constexpr size_t kBytesPerSample = sizeof(int16_t);
constexpr uint32_t kMaxRecordSeconds = 15;
constexpr size_t kMaxRecordBytes =
    kSampleRate * kChannels * kBytesPerSample * kMaxRecordSeconds;
constexpr size_t kIoChunkBytes = 480 * kBytesPerSample;

const char *TAG = "demo";

struct PsramDeleter {
    void operator()(uint8_t *buffer) const { heap_caps_free(buffer); }
};

using PsramBuffer = std::unique_ptr<uint8_t, PsramDeleter>;

}  // namespace

void demo_print_status() {
    bsp_power_status_t power{};
    if (bsp_power_get_status(&power) == ESP_OK) {
        ESP_LOGI(TAG,
                 "battery=%d%%/%umV present=%d charging=%d vbus=%umV system=%umV "
                 "power_on=%d power_off=%d gx8002=%s",
                 power.battery_percent, power.battery_mv, power.battery_present,
                 power.charging, power.vbus_mv, power.system_mv, power.power_on_source,
                 power.power_off_source, bsp_wakeup_version().c_str());
    }
}

void demo_run_self_test() {
    demo_print_status();
    ESP_LOGI(TAG, "self-test: white LED flash and motor pulse");
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_led_flash_white(250));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_motor_pulse(200));
}

void demo_run_wakeup_feedback() {
    ESP_LOGI(TAG, "GX8002 wake word detected: motor feedback");
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_motor_pulse(500, 1023));
}

void demo_record_and_playback() {
    PsramBuffer recording(static_cast<uint8_t *>(
        heap_caps_malloc(kMaxRecordBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
    if (!recording) {
        ESP_LOGE(TAG, "cannot allocate %u-byte recording buffer in PSRAM",
                 static_cast<unsigned>(kMaxRecordBytes));
        return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_led_set_rgb(32, 0, 0));
    ESP_LOGI(TAG, "recording started; release voice button to play back");

    size_t recorded_bytes = 0;
    while (bsp_button_is_pressed(BSP_BUTTON_VOICE) && recorded_bytes < kMaxRecordBytes) {
        const size_t bytes = std::min(kIoChunkBytes, kMaxRecordBytes - recorded_bytes);
        if (bsp_audio_read(recording.get() + recorded_bytes, bytes) != ESP_OK) {
            ESP_LOGE(TAG, "audio read failed after %u bytes",
                     static_cast<unsigned>(recorded_bytes));
            break;
        }
        recorded_bytes += bytes;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_led_off());
    if (recorded_bytes == kMaxRecordBytes) {
        ESP_LOGW(TAG, "maximum recording duration reached (%u seconds)",
                 static_cast<unsigned>(kMaxRecordSeconds));
    }
    if (recorded_bytes == 0) {
        ESP_LOGW(TAG, "nothing recorded");
        return;
    }

    int32_t peak = 0;
    const auto *samples = reinterpret_cast<const int16_t *>(recording.get());
    const size_t sample_count = recorded_bytes / sizeof(int16_t);
    for (size_t i = 0; i < sample_count; ++i) {
        const int32_t sample = samples[i];
        const int32_t magnitude = sample < 0 ? -sample : sample;
        peak = std::max(peak, magnitude);
    }

    ESP_LOGI(TAG, "recording stopped: %u bytes, peak=%ld; starting playback",
             static_cast<unsigned>(recorded_bytes), static_cast<long>(peak));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_led_set_rgb(0, 32, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_motor_start(1023));

    size_t played_bytes = 0;
    while (played_bytes < recorded_bytes) {
        const size_t bytes = std::min(kIoChunkBytes, recorded_bytes - played_bytes);
        if (bsp_audio_write(recording.get() + played_bytes, bytes) != ESP_OK) {
            ESP_LOGE(TAG, "audio write failed after %u bytes",
                     static_cast<unsigned>(played_bytes));
            break;
        }
        played_bytes += bytes;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_motor_stop());
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_led_off());
    ESP_LOGI(TAG, "playback finished: %u bytes", static_cast<unsigned>(played_bytes));
}
