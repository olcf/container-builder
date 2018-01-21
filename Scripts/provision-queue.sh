#!/bin/bash

set -e
set -o xtrace

# Create non root queue user
useradd --create-home --home-dir /home/queue --shell /bin/bash queue

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
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" ..
make
make install
cd /
rm -rf /container-builder

# Install OpenStack command line client
pip install python-openstackclient

# Create systemd script and launch the BuilderQueue daemon
cat << EOF > /etc/systemd/system/builder-queue.service
[Unit]
Description=builder-queue daemon
After=network.target

[Service]
Type=simple
User=queue
Environment="LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib"
WorkingDirectory=/home/queue
ExecStart=/usr/local/bin/builder-queue
Restart=no

[Install]
WantedBy=multi-user.target
EOF

# There appears to be some weird issues with starting systemd services inside of a cloud-init script
# The easiest thing to do is just reboot after enabling the service
systemctl enable builder-queue