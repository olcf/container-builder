Spack Package Repository
========================

In order to integrate container-builder with an existing Spack installation,
we separate the container-builder package into its own "Spack Repository" which
can be dynamically added to an existing spack installation like this:


```bash
# Assumes a loaded spack environment
spack repo add spack-repo/container-builder
spack install container-builder
```

This repository is structured according to Spack's repository definitions. For
example, see [Spack itself](https://github.com/spack/spack/tree/develop/var/spack/repos/builtin.mock).
