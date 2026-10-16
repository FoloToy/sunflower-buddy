#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
static() {
 python3 "$root/tools/check_boundaries.py"
 python3 "$root/tools/check_repo.py"
 python3 -m unittest discover -s "$root/tests" -p 'test_*.py'
}
case "${1:-all}" in
 --static) static ;;
 --firmware) "$root/tools/build.sh" ;;
 all) static; "$root/tools/build.sh" ;;
 *) echo "usage: $0 [--static|--firmware]" >&2; exit 2 ;;
esac
