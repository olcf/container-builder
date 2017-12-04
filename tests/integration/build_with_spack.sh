#!/bin/bash -ex
SPACKROOT=tests/artifacts/.spack

if [ ! -d $SPACKROOT ]; then
	git clone https://github.com/spack/spack.git $SPACKROOT
fi

$SPACKROOT/bin/spack repo add spack-repo/containerbuilder
$SPACKROOT/bin/spack install container-builder
