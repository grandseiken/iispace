#!/bin/bash
set -e

(which ${1} 2> /dev/null) && REPLAY_TOOL_A=${1}
(which ${2} 2> /dev/null) && REPLAY_TOOL_B=${2}
(which ${1} 2> /dev/null) || REPLAY_TOOL_A="bazel-out/${1}/bin/game/replay"
(which ${2} 2> /dev/null) || REPLAY_TOOL_B="bazel-out/${2}/bin/game/replay"
REPLAY_FILE="${3}"
TICK_START="${4}"
TICK_END="${5}"
TICK_INCREMENT="${6}"

echo "Finding diff between ${REPLAY_TOOL_A} and ${REPLAY_TOOL_B} of ${REPLAY_FILE}..."
echo "Ticks from ${TICK_START} to ${TICK_END} incrementing by ${TICK_INCREMENT}:"

TMP_DIR=$(mktemp -d)
for ((I=TICK_START; I <= TICK_END; I += TICK_INCREMENT)); do
  echo "Checking tick ${I}"
  DUMP_A="${TMP_DIR}/${I}_a.txt"
  DUMP_B="${TMP_DIR}/${I}_b.txt"
  "${REPLAY_TOOL_A}" "${REPLAY_FILE}" "--dump_portable" "--dump_tick=${I}" > "${DUMP_A}"
  "${REPLAY_TOOL_B}" "${REPLAY_FILE}" "--dump_portable" "--dump_tick=${I}" > "${DUMP_B}"
  echo "    diff ${DUMP_A} ${DUMP_B}"
  diff "${DUMP_A}" "${DUMP_B}" || (echo "Found diff at tick ${I}" && exit 1)
done
