#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

sudo apt update
sudo apt install -y tar net-tools gcc-multilib g++-multilib ninja-build libpcap-dev ethtool isc-dhcp-server unifdef dos2unix
