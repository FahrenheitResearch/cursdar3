#!/usr/bin/env bash
set -euo pipefail

echo "=== CURSDAR3 Build ==="

cd "$(dirname "$0")"

mkdir -p build
cd build

cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .

echo
echo "=== Build successful ==="
