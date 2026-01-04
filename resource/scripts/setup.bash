#!/usr/bin/env bash

SCRIPT_PATH=$(builtin cd "`dirname "${BASH_SOURCE[0]}"`" > /dev/null && pwd)
ROOT_DIR=$(cd "${SCRIPT_PATH}/../.."; pwd)

COLOR_WHITE="\033[37m"
COLOR_GREEN="\033[32m"
COLOR_GRAY="\033[90m"
COLOR_BLUE="\033[34m"
COLOR_RESET="\033[0m"

print_delim() {
    echo "===================================================="
}

print_banner_box() {
    local text="$1"
    local border="===================================================="
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")

    echo -e "${COLOR_WHITE}${border}${COLOR_RESET}"
    printf "${COLOR_GREEN}%*s%s%*s${COLOR_RESET}\n" \
        $(( (50 - ${#text}) / 2 )) "" "$text" $(( (50 - ${#text} + 1) / 2 )) ""
    echo -e ""

    echo -e "${COLOR_GRAY}Copyright (C) 2025 T3CAIC Group Limited."
    echo -e "All rights reserved.${COLOR_RESET}"
    echo -e "${COLOR_GREEN}Started at ${timestamp}${COLOR_RESET}"
}

# ================= 环境变量设置 ================= #
export PROJECT="dcp plus"
export VIN="VIN123456789"
export INSTALL_ROOT_PATH=$ROOT_DIR
export CAR_ID=NIO-ES6
export LD_LIBRARY_PATH=$ROOT_DIR/lib:$LD_LIBRARY_PATH

print_banner_box "$PROJECT"
print_delim
