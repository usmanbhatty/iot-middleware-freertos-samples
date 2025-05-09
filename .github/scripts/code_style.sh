#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

usage() {
    echo "${0} [check|fix]" 1>&2
    exit 1
}

FIX=${1:-""}

# Version 0.67 is the source of truth
if ! [ -x "$(command -v uncrustify)" ]; then
    tmp_dir=$(mktemp -d -t uncrustify-XXXX)
    pushd $tmp_dir
    git clone https://github.com/uncrustify/uncrustify.git
    cd uncrustify
    git checkout uncrustify-0.67
    mkdir build
    cd build
    cmake ..
    cmake --build .
    sudo make install
    popd
fi

if [[ "$FIX" == "check" ]]; then
    RESULT=$(uncrustify -c ./uncrustify.cfg --check $(find demos -name "*.[ch]"      \
    -not -path "*nxp_code*"                                                          \
    -not -path "*WinPCap*"                                                           \
    -not -path "*st_code*"                                                           \
    -not -path "*components*"))

    if [ $? -ne 0 ]; then
      echo $RESULT | grep "FAIL"
      exit 1
    fi
elif [[ "$FIX" == "fix" ]]; then
    uncrustify -c ./uncrustify.cfg --no-backup --replace $(find demos -name "*.[ch]" \
    -not -path "*nxp_code*"                                                          \
    -not -path "*WinPCap*"                                                           \
    -not -path "*st_code*"                                                           \
    -not -path "*components*")
else
    usage
fi
