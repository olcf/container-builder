#!/bin/bash

set -e
set -o xtrace

VERSION=$1

export HOME=$(pwd)

source ${MODULESHOME}/init/bash
export PATH=$PATH:${MODULESHOME}/bin

module unload DefApps

set -x

SW_ROOT=/sw/summitdev/container-builder/${VERSION}
mkdir -p ${SW_ROOT}

# Build with spack; install spack if it's not already present.
SPACKROOT=${SW_ROOT}/.spack
if [ ! -d ${SPACKROOT} ]; then
	git clone https://github.com/spack/spack.git ${SPACKROOT}
	cd ${SPACKROOT}
	git checkout d3519af7de84fa72dee0618c7754f7ebeaa23142
	cd ${HOME}
fi

# Update the spack configuration files to the latest.
cp spack-etc-summitdev/*.yaml ${SPACKROOT}/etc/spack

# Ensure spack is aware of our third-party package repo.
${SPACKROOT}/bin/spack repo add spack-repo/container-builder

# Remove any remnants of past failed builds.
${SPACKROOT}/bin/spack clean --stage --misc-cache

# Record how spack will build the app in the log.
${SPACKROOT}/bin/spack spec -NIl "container-builder%gcc@7.1.0"

# Have spack build the app.
${SPACKROOT}/bin/spack install "container-builder%gcc@7.1.0"

# Find where spack generated the modulefile.
root=$(${SPACKROOT}/bin/spack config get config | grep "\btcl:" | awk '{print $2}' | sed 's/^$spack/./')
arch=$(${SPACKROOT}/bin/spack arch)
mfname=$(${SPACKROOT}/bin/spack module find "container-builder%gcc@7.1.0")
real_mf_path="$SPACKROOT/$root/$arch/$mfname"

# Generate a public modulefile the loads the spack-generated one.
MF_ROOT=/sw/summitdev/modulefiles/core/container-builder
mkdir -p ${MF_ROOT}

source artifacts/queue-host.sh

cat << EOF > ${MF_ROOT}/${VERSION}
#%Module

setenv QUEUE_HOST ${QUEUE_HOST}
setenv QUEUE_PORT 8080
module --ignore-cache load ${real_mf_path}
EOF
