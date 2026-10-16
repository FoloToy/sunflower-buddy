#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$root/tools/check_boundaries.py"
python3 "$root/tests/run_button_tests.py"
command -v idf.py >/dev/null
idf.py --version | grep -q '5\.5\.3'
idf.py -C "$root" build
idf.py -C "$root/examples/hardware-demo" build
