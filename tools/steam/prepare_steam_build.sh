#!/bin/bash
# Build successfully before running, e.g.
#   bazel build --config clang-cl --steam ...
# After running:
#   - bazel run @steamworks_sdk//:steamcmd
#   - login <username> <password>
#   - run_app_build <repo_path>/tools/steam/app_build.vdf
set -e

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
ROOT_DIR="${SCRIPT_DIR}/../.."
CONTENT_DIR="${ROOT_DIR}/steam_build/content"
rm -rf ${CONTENT_DIR}
mkdir -p ${CONTENT_DIR}
unzip "${ROOT_DIR}/bazel-bin/game/game.zip" "-d" "${CONTENT_DIR}"
