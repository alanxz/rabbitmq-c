#!/usr/bin/env bash

# Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
# SPDX-License-Identifier: mit

build_cmake() {
  sudo apt install -y xmlto
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DBUILD_TOOLS_DOCS=ON -DCMAKE_INSTALL_PREFIX=$PWD/../_install -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Wno-implicit-fallthrough -Werror" 
  cmake --build . --target install
  ctest -V .
}

build_framing() {
  sudo apt install -y clang-format
  ./regenerate_framing.sh
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DCMAKE_INSTALL_PREFIX=$PWD/../_install -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Wno-implicit-fallthrough -Werror" 
  cmake --build . --target install
  ctest -V .
}

build_macos() {
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DCMAKE_INSTALL_PREFIX=$PWD/../_install -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Werror" \
    -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl@1.1"
  cmake --build . --target install
  ctest -V .
}

build_format() {
  sudo apt-get install -y clang-format
  ./travis/run-clang-format/run-clang-format.py \
    --clang-format-executable="${PWD}/travis/clang-format.sh" \
    --recursive examples librabbitmq tests tools
}

build_coverage() {
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Coverage -DCMAKE_INSTALL_PREFIX=$PWD/../_install \
    -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Werror -fprofile-arcs -ftest-coverage"
  cmake --build . --target install
  ctest -V .
  
  pip install --user cpp-coveralls
  coveralls --exclude tests --build-root . --root .. --gcov-options '\-lp'
}

build_asan() {
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/../_install \
    -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Werror -fsanitize=address,undefined -O1"
  cmake --build . --target install
  ctest -V .
}

build_tsan() {
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/../_install \
    -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Werror -fsanitize=thread,undefined -O1"
  cmake --build . --target install
  ctest -V .
}

build_scan-build() {
  sudo apt install -y clang-tools
  mkdir $PWD/_build && cd $PWD/_build
  scan-build cmake .. -GNinja -DBUILD_EXAMPLES=ON -DBUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_INSTALL_PREFIX=$PWD/../_install \
    -DCMAKE_C_FLAGS="-Wall -Wextra -Wstrict-prototypes -Wno-unused-function -Werror"
  scan-build ninja install
}

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 {cmake|framing|macos|format|coverage|asan|tsan|scan-build}"
  exit 1
fi

set -e  # exit on error.
set -x  # echo commands.

eval "build_$1"
