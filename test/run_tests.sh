#!/usr/bin/env bash
# Build + run all native (host g++) unit tests for the pure-logic modules.
# These need no hardware. Run from anywhere in Git Bash:
#     ./test/run_tests.sh
#
# Requires w64devkit's g++ (see the project memory). If g++ isn't already on
# PATH we prepend the known install location.
set -euo pipefail

W64="/c/Users/PC/tools/w64devkit/bin"
command -v g++ >/dev/null 2>&1 || export PATH="$W64:$PATH"
command -v g++ >/dev/null 2>&1 || { echo "g++ not found (expected at $W64)"; exit 1; }

# Resolve the sketch root as this script's parent directory.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXXFLAGS="-std=c++11 -I. -Wall -Wextra"
fail=0

build_run() {
  local name="$1"; shift            # test binary name; remaining args = sources
  echo "== $name =="
  g++ $CXXFLAGS "$@" -o "test/$name.exe"
  "./test/$name.exe" || fail=1
  echo
}

build_run run_sunrise SunriseCurve.cpp                 test/test_sunrise_curve.cpp
build_run run_light   LightEngine.cpp                  test/test_light_engine.cpp
build_run run_alarm   AlarmSequence.cpp SunriseCurve.cpp test/test_alarm_sequence.cpp
build_run run_ui      UIMenu.cpp                       test/test_ui_menu.cpp

if [ "$fail" -eq 0 ]; then echo "==> ALL SUITES PASS"; else echo "==> SOME SUITES FAILED"; exit 1; fi
