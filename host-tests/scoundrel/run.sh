#!/bin/sh
set -e
cd "$(dirname "$0")"
BUILD_DIR="${TMPDIR:-/tmp}/$(basename "${CXX:-c++}")-scoundrel-tests-$(cd ../.. && pwd | cksum | cut -d" " -f1)"
mkdir -p "$BUILD_DIR"
SRC=../../src/apps_local/scoundrel
SOLITAIRE_SRC=../../src/apps_local/solitaire
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -O2 $SOLITAIRE_SRC/SolitaireCore.cpp $SRC/ScoundrelState.cpp \
  test_scoundrel.cpp -o "$BUILD_DIR/test_scoundrel"
"$BUILD_DIR/test_scoundrel"
