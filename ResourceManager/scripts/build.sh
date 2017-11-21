#!/bin/bash

# Create loop device
losetup -f

WORKD_DIR=`pwd`

# Build container
SINGULARITY_CACHEDIR=${WORK_DIR}
/usr/local/bin/singularity build ${WORK_DIR}/container.img ${WORK_DIR}/container.def