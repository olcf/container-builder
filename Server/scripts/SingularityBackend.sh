#!/bin/bash

set -e

# Create non root builder user
useradd --create-home --home-dir /home/builder --shell /bin/bash builder

# Allow builder to run singularity as root
echo 'builder ALL=(ALL) NOPASSWD: /usr/local/bin/singularity' > /etc/sudoers.d/builder
chmod 0440 /etc/sudoers.d/builder

# Copy over singularity build script
cp /build.sh /home/builder/build.sh
chmod +x /home/builder/build.sh

# Create builder scratch work directory
mkdir /home/builder/container_scratch
chown builder /home/builder/container_scratch
chgrp builder /home/builder/container_scratch

# Install Singularity
export VERSION=2.4
wget https://github.com/singularityware/singularity/releases/download/$VERSION/singularity-$VERSION.tar.gz
tar xvf singularity-$VERSION.tar.gz
cd singularity-$VERSION
./configure --prefix=/usr/local
make
make install

# Create builder service
cd /home/cades
git clone https://github.com/AdamSimpson/ContainerBuilder.git
cd ContainerBuilder/Server/scripts
su builder -c 'sudo singularity build /home/builder/builder.img builder.def'

# Start the container builder service
cd /home/builder
su builder -c 'singularity instance.start ./builder.img builder'
