#!/bin/bash -e
VERSION=$1

source $MODULESHOME/init/bash
export PATH=$PATH:$MODULESHOME/bin

module unload xalt PrgEnv-pgi PrgEnv-gnu PrgEnv-intel PrgEnv-cray
module load PrgEnv-gnu
module swap gcc gcc/7.1.0

set -x
rm -rf $HOME/.spack

unset CRAYPE_VERSION
SW_ROOT=/sw/xk6/container-builder/
mkdir -p $SW_ROOT

SPACKROOT=$SW_ROOT/.spack

if [ ! -d $SPACKROOT ]; then
	git clone https://github.com/spack/spack.git $SPACKROOT
fi
cp spack-etc/*.yaml $SPACKROOT/etc/spack

$SPACKROOT/bin/spack repo add spack-repo/containerbuilder
$SPACKROOT/bin/spack install "container-builder@$VERSION%gcc@7.1.0 ^cmake@3.9.0%gcc@4.7+ownlibs"

root=$($SPACKROOT/bin/spack config get config | grep "\btcl:" | awk '{print $2}' | sed 's/^$spack/./')
arch=$($SPACKROOT/bin/spack arch)
mfname=$($SPACKROOT/bin/spack module find "container-builder%gcc@7.1.0")

real_mf_path="$SPACKROOT/$root/$arch/$mfname"

MF_ROOT=/sw/xk6/modulefiles/container-builder
mkdir -p $MF_ROOT
cat << EOF > $MF_ROOT/$VERSION
#%Module

setenv QUEUE_HOST 128.219.186.173
setenv QUEUE_PORT 8080
load $real_mf_path
EOF
