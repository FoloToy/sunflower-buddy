#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mode="${1:-all}"

run_static() {
    python3 "${project_root}/tools/check_repo.py"
    python3 -m unittest discover -s "${project_root}/tests" -p 'test_*.py'
}

run_firmware() {
    if [[ -z "${IDF_PATH:-}" ]]; then
        echo "IDF_PATH is not set; activate ESP-IDF 5.5.3 first" >&2
        return 2
    fi
    if ! idf.py --version | grep -q '5\.5\.3'; then
        echo "ESP-IDF 5.5.3 is required" >&2
        return 2
    fi

    validation_dir="$(mktemp -d /tmp/sunflower-buddy-validation.XXXXXX)"
    trap 'rm -rf "${validation_dir}"' EXIT
    idf.py -C "${project_root}" \
        -B "${validation_dir}/build" \
        -D "SDKCONFIG=${validation_dir}/sdkconfig" \
        -D "SDKCONFIG_DEFAULTS=${project_root}/sdkconfig.defaults" \
        set-target esp32s3 build
}

case "${mode}" in
    --static)
        run_static
        ;;
    --firmware)
        run_firmware
        ;;
    all)
        run_static
        run_firmware
        ;;
    *)
        echo "usage: $0 [--static|--firmware]" >&2
        exit 2
        ;;
esac
