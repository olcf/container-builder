#!/bin/bash

set -e
set -o xtrace

# Create non root queue user
useradd --create-home --home-dir /home/queue --shell /bin/bash queue

apt-get -y update
apt-get -y install unzip

# Setup firewall
ufw default deny incoming
ufw default allow outgoing
ufw allow ssh
ufw allow from 160.91.205.192/26 to any port 8080 # Titan
ufw allow from 128.219.141.227 to any port 8080   # Summitdev
ufw allow from 128.219.134.71/26 to any port 8080 # Summit
ufw --force enable

# Update cmake version
cd /
wget -q https://cmake.org/files/v3.9/cmake-3.9.5-Linux-x86_64.sh
chmod +x ./cmake-3.9.5-Linux-x86_64.sh
./cmake-3.9.5-Linux-x86_64.sh --skip-license
ln -s /cmake-3.9.5-Linux-x86_64/bin/* /usr/local/bin
cd /
rm -rf /cmake-3.9.5-Linux-x86_64

# Install a new version of boost
wget -q https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar xf boost_1_66_0.tar.gz
cd boost_1_66_0
./bootstrap.sh --with-libraries=filesystem,regex,system,serialization,thread,program_options
./b2 install || :
cd /
rm -rf /boost_1_66_0

# Install ContainerBuilder
cd /
git clone https://code.ornl.gov/olcf/container-builder.git
cd container-builder
mkdir build && cd build
cmake -DCOMPONENT=builder-queue -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" ..
make builder-queue
cmake -DCOMPONENT=builder-queue -P cmake_install.cmake
cd /
rm -rf /container-builder

# Install OpenStack command line client
pip install python-openstackclient

# Install docker-ls
wget https://github.com/mayflower/docker-ls/releases/download/v0.3.1/docker-ls-linux-amd64.zip
unzip docker-ls-linux-amd64
mv docker-ls /usr/local/bin
rm docker-rm

# Create systemd script and launch the BuilderQueue daemon
cat << EOF > /etc/systemd/system/builder-queue.service
[Unit]
Description=builder-queue daemon
After=network.target

[Service]
Type=simple
User=queue
EnvironmentFile=/home/queue/environment.sh
WorkingDirectory=/home/queue
ExecStart=/usr/local/bin/builder-queue
Restart=no

[Install]
WantedBy=multi-user.target
EOF

systemctl enable builder-queue