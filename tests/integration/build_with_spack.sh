#!/bin/bash -e

source $MODULESHOME/init/bash
export PATH=$PATH:$MODULESHOME/bin

module unload xalt PrgEnv-pgi PrgEnv-gnu PrgEnv-intel PrgEnv-cray
module load PrgEnv-gnu
module swap gcc gcc/7.1.0

set -x
rm -rf $HOME/.spack

unset CRAYPE_VERSION
SPACKROOT=tests/artifacts/.spack

if [ ! -d $SPACKROOT ]; then
	git clone https://github.com/spack/spack.git $SPACKROOT
fi

$SPACKROOT/bin/spack repo add spack-repo/containerbuilder
$SPACKROOT/bin/spack install "container-builder%gcc@7.1.0 ^cmake@3.9.0%gcc@4.7+ownlibs"
