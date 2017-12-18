#!/bin/bash -e
VERSION=$1

export HOME=$(pwd)

source $MODULESHOME/init/bash
export PATH=$PATH:$MODULESHOME/bin

module unload DefApps

set -x

SW_ROOT=/sw/summitdev/container-builder/$VERSION
mkdir -p $SW_ROOT

SPACKROOT=$SW_ROOT/.spack

if [ ! -d $SPACKROOT ]; then
	git clone https://github.com/spack/spack.git $SPACKROOT
fi
cp spack-etc-summitdev/*.yaml $SPACKROOT/etc/spack

$SPACKROOT/bin/spack repo add spack-repo/containerbuilder
$SPACKROOT/bin/spack spec -NIl "container-builder%gcc@7.1.0"
$SPACKROOT/bin/spack install "container-builder%gcc@7.1.0"

root=$($SPACKROOT/bin/spack config get config | grep "\btcl:" | awk '{print $2}' | sed 's/^$spack/./')
arch=$($SPACKROOT/bin/spack arch)
mfname=$($SPACKROOT/bin/spack module find "container-builder%gcc@7.1.0")

real_mf_path="$SPACKROOT/$root/$arch/$mfname"

MF_ROOT=/sw/summitdev/modulefiles/container-builder
mkdir -p $MF_ROOT
cat << EOF > $MF_ROOT/$VERSION
#%Module

setenv QUEUE_HOST 128.219.186.173
setenv QUEUE_PORT 8080
module load $real_mf_path
EOF