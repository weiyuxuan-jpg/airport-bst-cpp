#!/bin/sh
# Builds and runs every tests/test_*.cpp against the project sources (all of src/ except
# main.cpp). Run from the project root:
#   sh tests/run_tests.sh              normal -O2 build
#   SANITIZE=1 sh tests/run_tests.sh   with AddressSanitizer + UndefinedBehaviorSanitizer
# Set SDKROOT first if your macOS toolchain needs it (see PROGRESS.md, R1).
set -e

CXX=${CXX:-g++}
FLAGS="-std=c++17 -Wall -Wextra -pedantic -Isrc -Itests"
if [ "${SANITIZE:-0}" = "1" ]; then
    FLAGS="$FLAGS -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer"
else
    FLAGS="$FLAGS -O2"
fi
SOURCES=$(ls src/*.cpp | grep -v '^src/main.cpp$')

mkdir -p tests/bin
status=0
for test in tests/test_*.cpp; do
    name=$(basename "$test" .cpp)
    echo "== $name =="
    $CXX $FLAGS "$test" $SOURCES -o "tests/bin/$name"
    if ! "./tests/bin/$name"; then status=1; fi
done
exit $status
