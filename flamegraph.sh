#!/bin/bash
set -e

TMP_DIR=$(mktemp -d)
FLAMEGRAPH="$(bazel info output_base)/external/flamegraph"
OUT_SVG="${TMP_DIR}/$(basename $1).svg"
bazel build --config gcc "@flamegraph//:flamegraph" "//game:score"
perf record -o "${TMP_DIR}/perf.data" -g bazel-bin/game/score "$1" > /dev/null
perf script -i "${TMP_DIR}/perf.data" > "${TMP_DIR}/out.perf"
"${FLAMEGRAPH}/stackcollapse-perf.pl" "${TMP_DIR}/out.perf" > "${TMP_DIR}/out.folded"
"${FLAMEGRAPH}/flamegraph.pl" "${TMP_DIR}/out.folded" > "${OUT_SVG}"
echo "${OUT_SVG}"
