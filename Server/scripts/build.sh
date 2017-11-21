#!/bin/bash

# Create loop device
losetup -f

# Build container
sudo /usr/local/bin/singularity build $1/container.img $1/container.def
