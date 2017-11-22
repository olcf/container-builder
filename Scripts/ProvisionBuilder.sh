#!/usr/bin/env bash

set -e
set -o xtrace

#apt update
#apt -y upgrade

# Create non root builder user
useradd --create-home --home-dir /home/builder --shell /bin/bash builder

# Allow builder to run singularity as root
echo 'builder ALL=(ALL) NOPASSWD: /usr/local/bin/singularity' > /etc/sudoers.d/builder
chmod 0440 /etc/sudoers.d/builder

# Install Singularity
export VERSION=2.4
wget https://github.com/singularityware/singularity/releases/download/$VERSION/singularity-$VERSION.tar.gz
tar xvf singularity-$VERSION.tar.gz
cd singularity-$VERSION
./configure --prefix=/usr/local
make
make install

# Update cmake version
cd /
wget https://cmake.org/files/v3.9/cmake-3.9.5-Linux-x86_64.sh
chmod +x ./cmake-3.9.5-Linux-x86_64.sh
./cmake-3.9.5-Linux-x86_64.sh --skip-license
ln -s /cmake-3.9.5-Linux-x86_64/bin/* /usr/local/bin
rm -rf /cmake-3.9.5-Linux-x86_64

# Install a new version of boost
cd /
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
tar xf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh --with-libraries=filesystem,regex,system,coroutine,serialization,log,thread
./b2 install || :
rm -rf /boost_1_65_1

# Install ContainerBuilder
cd /
git clone https://github.com/AdamSimpson/ContainerBuilder.git
cd ContainerBuilder
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" ..
make
make install
rm -rf /ContainerBuilder

# Create systemd script and launch the Builder daemon
cat << EOF > /etc/systemd/system/Builder.service
[Unit]
Description=ContainerBuilder daemon
After=network.target

[Service]
Type=simple
User=queue
WorkingDirectory=/home/builder
Environment="QUEUE_HOSTNAME=${QUEUE_HOSTNAME}"
Environment="QUEUE_PORT=${QUEUE_PORT}"
ExecStart=/usr/local/bin/ContainerBuilder
Restart=no

[Install]
WantedBy=multi-user.target
EOF
systemctl start Builder