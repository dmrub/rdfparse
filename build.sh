#!/usr/bin/env bash

set -eo pipefail

THIS_DIR=$(cd "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

set -x
BUILD_DIR=$THIS_DIR/build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$THIS_DIR"
make -j2
