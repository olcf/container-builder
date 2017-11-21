#!/bin/bash

# Create loop device
losetup -f

# Build container
sudo /usr/local/bin/singularity build ./container.img ./container.def $1
