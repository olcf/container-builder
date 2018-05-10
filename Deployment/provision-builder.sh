#!/bin/bash

set -e
set -o xtrace

# Create non root builder user
useradd --create-home --home-dir /home/builder --shell /bin/bash builder

# Allow builder to run singularity as root
echo 'builder ALL=(ALL) NOPASSWD: /usr/local/bin/singularity-builder-backend.sh' > /etc/sudoers.d/builder
echo 'builder ALL=(ALL) NOPASSWD: /usr/local/bin/docker-builder-backend.sh' >> /etc/sudoers.d/builder
chmod 0440 /etc/sudoers.d/builder

apt-get -y update
apt-get -y install expect
apt-get -y install yum rpm autogen autoconf libtool libarchive-dev

# Install docker
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
apt-get update
apt-get install -y docker-ce

# Use overlay2 to get around issues with aufs .wh files blowing up singularity
sudo systemctl stop docker
cat << EOF > /etc/docker/daemon.json
{
  "storage-driver": "overlay2"
}
EOF
sudo systemctl start docker

# Setup firewall
ufw default deny incoming
ufw default allow outgoing
ufw allow ssh
ufw allow from 160.91.205.192/26 to any port 8080 # Titan
ufw allow from 128.219.141.227 to any port 8080   # Summitdev
ufw allow from 128.219.134.71/26 to any port 8080 # Summit
ufw --force enable

#####################################
# begin ppc64le QUEMU stuff
######################################

# Add support of ppc64le arch
apt-get install -y qemu-user-static binfmt-support zlib1g-dev libglib2.0-dev libpixman-1-dev libfdt-dev libpython2.7-stdlib
apt-get clean
rm -rf /var/lib/apt/lists/*

# Install a newer qemu from source to support ppc64le
# This makes a very long time so perhaps just build the ppc64le component?
wget -q 'https://download.qemu.org/qemu-2.11.1.tar.xz'
tar xvJf qemu-2.11.1.tar.xz
cd qemu-2.11.1
mkdir build
cd build
../configure --static --target-list=ppc64le-linux-user
make
make install

# A dirty hack to copy the custom qemu static binary(ies) over top of the distro provided dynamically linked binaries
cp -r /usr/local/bin/qemu-ppc64le /usr/bin/qemu-ppc64le-static

#####################################
# End ppc64le
#####################################

# fix yum/RPM issue under debian: https://github.com/singularityware/singularity/issues/241
cat << EOF > /root/.rpmmacros
%_var /var
%_dbpath %{_var}/lib/rpm
EOF

# Install Singularity
apt-get install -y libarchive-dev
cd /
git clone https://github.com/singularityware/singularity.git
cd singularity
git fetch
git checkout release-2.5
./autogen.sh
./configure --prefix=/usr/local
make
make install

# Update cmake version
cd /
wget -q https://cmake.org/files/v3.9/cmake-3.9.5-Linux-x86_64.sh
chmod +x ./cmake-3.9.5-Linux-x86_64.sh
./cmake-3.9.5-Linux-x86_64.sh --skip-license
ln -s /cmake-3.9.5-Linux-x86_64/bin/* /usr/local/bin
cd /
rm -rf /cmake-3.9.5-Linux-x86_64

# Install a new version of boost
cd /
wget -q https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar xf boost_1_66_0.tar.gz
cd boost_1_66_0
./bootstrap.sh --with-libraries=filesystem,regex,system,serialization,thread,program_options
./b2 install || :
rm -rf /boost_1_66_0

# Install builder_server
cd /
git clone https://code.ornl.gov/olcf/container-builder.git
cd container-builder
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/usr/local" ..
make builder-server
cmake -DCOMPONENT=builder-server -P cmake_install.cmake
cd /
rm -rf /container-builder

# Create systemd script and launch the Builder daemon
cat << EOF > /etc/systemd/system/builder-server.service
[Unit]
Description=builder-server daemon
After=network.target

[Service]
Type=simple
User=builder
WorkingDirectory=/home/builder
EnvironmentFile=/home/builder/environment.sh
ExecStart=/usr/local/bin/builder-server
Restart=no

[Install]
WantedBy=multi-user.target
EOF

systemctl enable builder-server
