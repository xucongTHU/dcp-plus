#!/bin/sh

export VERSION_TMP="/tmp"
if [ -n "$kVersionTmpPath" ];then
    echo "$kVersionTmpPath"
    VERSION_TMP="$kVersionTmpPath"
fi

#set -ex

. $1/tmp/senseauto_data_closed_loop/release_helper.sh
cp -rf $1/tmp/* ${VERSION_TMP}
INSTALL_CUR_VERSION "shadow_mode"

cp -rf $1/opt/senseauto/tmp/senseauto_data_closed_loop/* ${INSTALL_DIR}
cp -rf $1/tmp/senseauto_data_closed_loop/release.conf.json ${INSTALL_DIR}

echo "${ACTIVE_DIR}/lib/" > /etc/ld.so.conf.d/dcp.conf
echo "${BASE_ROOT}/senseauto/senseauto-msgs/active/lib/" >> /etc/ld.so.conf.d/dcp.conf

#ldconfig
