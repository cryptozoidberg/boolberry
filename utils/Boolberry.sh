#!/bin/bash
set -e
curr_path=${BASH_SOURCE%/*}

set -x
LD_LIBRARY_PATH=$curr_path/lib QT_PLUGIN_PATH=$curr_path/lib $curr_path/Boolberry
