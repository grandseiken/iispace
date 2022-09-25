#!/bin/bash
set -e

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
ROOT_DIR="${SCRIPT_DIR}/../.."
CONTENT_DIR="${ROOT_DIR}/steam_build/content"
mkdir -p ${CONTENT_DIR}
unzip "${ROOT_DIR}/bazel-bin/game/game.zip" "-d" "${CONTENT_DIR}"
