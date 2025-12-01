#!/bin/bash
CURR_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR=$(cd "${CURR_DIR}/../";pwd)

HOST='10.77.5.68'
USER='caic01201'
PASSWORD='123456'

# 使用expect实现自动ssh登录并拷贝文件
/usr/bin/expect <<EOF
set timeout -1
spawn scp -r ${ROOT_DIR}/build/install/ $USER@$HOST:/home/nvidia/userdata/dataengine/
expect {
    "*password:" { send "$PASSWORD\r" }
}
expect eof
EOF

sshpass -p "$PASSWORD" ssh -t -t -o StrictHostKeyChecking=no $USER@$HOST <<EOF
	sshpass -p "nvidia" scp -r /home/nvidia/userdata/dataengine/install nvidia@192.168.1.196:/home/nvidia/userdata/dataengine
	exit
EOF
