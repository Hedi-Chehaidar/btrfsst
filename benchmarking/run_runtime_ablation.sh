#!/usr/bin/env bash

# Runtime-ablation benchmark for the camera-ready paper (Table: runtime ablation).
#
# For each ablation configuration (baseline, +dp-train, +triples, +prune,
# +dp-encode, full Opt) of FSST and FSST12, this measures per corpus file:
#   - TableSeconds:  symbol-table construction time (training only)
#   - EncodeSeconds: final corpus encoding time (excludes table construction)
# Each value is the average of 5 repetitions. End-to-end compression time is
# TableSeconds + EncodeSeconds.
#
# Requirements: cmake, g++ (C++20), and the benchmark corpus under data/refined.
# Output: benchmarking/csv/runtime_ablation.csv and runtime_ablation12.csv.

set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR_NOAVX="$ROOT_DIR/build-noavx512"
BUILD_DIR_12="$ROOT_DIR/build12"
BENCH_DIR="$ROOT_DIR/benchmarking"

step() {
    printf '\n==> %s\n' "$1"
}

fail() {
    printf 'Error: %s\n' "$1" >&2
    exit 1
}

trap 'fail "run_runtime_ablation.sh failed at line $LINENO"' ERR

[[ -d "$ROOT_DIR/data/refined" ]] || fail "benchmark corpus not found at data/refined"

step "Building scalar FSST binary"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR_NOAVX" -DFSST_DISABLE_AVX512=ON
cmake --build "$BUILD_DIR_NOAVX" -j --target binary

step "Building FSST12 binary"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR_12"
cmake --build "$BUILD_DIR_12" -j --target binary12

step "Building benchmark runner"
g++ -std=c++20 -O3 "$BENCH_DIR/runner.cpp" -o "$BENCH_DIR/runner"

step "Running runtime-ablation benchmark (takes roughly 15-45 minutes)"
(
    cd "$BENCH_DIR"
    ./runner --only-runtime-ablation
)

step "Done: results in $BENCH_DIR/csv/runtime_ablation.csv and runtime_ablation12.csv"
