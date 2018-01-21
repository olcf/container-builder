#!/bin/bash

set -e
set -o xtrace

# Disable TCP time stamps - ORNL seems to be jacking them up and causing the connection to drop at the 5 minute mark
#
echo 0 > /proc/sys/net/ipv4/tcp_timestamps # this session
cat << EOF > /etc/sysctl.d/51-net.conf     # permanent
net.ipv4.tcp_timestamps = 0
EOF