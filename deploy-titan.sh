#!/bin/bash

set -e
set -o xtrace

VERSION=$1

export HOME=$(pwd)

source ${MODULESHOME}/init/bash
export PATH=$PATH:${MODULESHOME}/bin

module unload xalt PrgEnv-pgi PrgEnv-gnu PrgEnv-intel PrgEnv-cray

set -x

unset CRAYPE_VERSION
SW_ROOT=/sw/xk6/container-builder/${VERSION}
mkdir -p ${SW_ROOT}

SPACKROOT=${SW_ROOT}/.spack

if [ ! -d ${SPACKROOT} ]; then
	git clone https://github.com/spack/spack.git ${SPACKROOT}
	cd ${SPACKROOT}
	git checkout d3519af7de84fa72dee0618c7754f7ebeaa23142
	cd ${HOME}
fi
cp spack-etc-titan/*.yaml ${SPACKROOT}/etc/spack

${SPACKROOT}/bin/spack repo add spack-repo/containerbuilder
${SPACKROOT}/bin/spack spec -NIl "container-builder%gcc@5.3.0"
${SPACKROOT}/bin/spack install "container-builder%gcc@5.3.0"

root=$(${SPACKROOT}/bin/spack config get config | grep "\btcl:" | awk '{print $2}' | sed 's/^$spack/./')
arch=$(${SPACKROOT}/bin/spack arch)
mfname=$(${SPACKROOT}/bin/spack module find "container-builder%gcc@5.3.0")

real_mf_path="$SPACKROOT/$root/$arch/$mfname"

MF_ROOT=/sw/xk6/modulefiles/container-builder
mkdir -p ${MF_ROOT}

source artifacts/queue_host.sh

cat << EOF > ${MF_ROOT}/${VERSION}
#%Module

setenv QUEUE_HOST ${QUEUE_HOST}
setenv QUEUE_PORT 8080
module load ${real_mf_path}
EOF
