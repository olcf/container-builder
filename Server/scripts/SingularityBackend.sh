#!/bin/bash

set -e

# Create non root builder user
useradd --create-home --home-dir /home/builder --shell /bin/bash builder

# Allow builder to run singularity as root
echo 'builder ALL=(ALL) NOPASSWD: /usr/local/bin/singularity' > /etc/sudoers.d/builder
chmod 0440 /etc/sudoers.d/builder

# Copy over singularity build script
wget https://raw.githubusercontent.com/AdamSimpson/ContainerBuilder/master/Server/scripts/builder.def
mv builder.def /home/builder/builder.def
wget https://raw.githubusercontent.com/AdamSimpson/ContainerBuilder/master/Server/scripts/build.sh
mv build.sh /home/builder/build.sh
chmod +x /home/builder/build.sh
wget https://raw.githubusercontent.com/AdamSimpson/ContainerBuilder/master/Server/scripts/singularity_backend.def
mv singularity_backend.def /home/builder/singularity_backend.def

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
su builder -c 'sudo singularity build /home/builder/builder.img /home/builder/builder.def'

# Start the container builder service
cd /home/builder
su builder -c 'singularity instance.start ./builder.img builder'
