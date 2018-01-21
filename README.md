container-builder
================
Containers traditionally require root access to build from a recipe file, as such native access to build cannot be granted on OLCF HPC resources such as Titan and Summit. 
container-builder is an interactive container building utility that gets around this limitation by building each container on a remote ephemeral VM, streaming the output in real time to the client.

Use
=================
Singularity recipe:
```
$ module load container-builder
$ container-builder container.img singularity.recipe
```
Docker recipe:
```
$ module load container-builder
$ container-builder --backend=docker container.img docker.recipe
```

OLCF Recipes
==================
container-builder has access to the private OLCF container-recipes docker registry.

Implementation
==================
Some insight into the build process:

* Client initiates build request through CLI
* Client build request enters the queue
* Queue creates builder
* Builder details sent to the client
* The client connects to the builder
* Container recipe file is sent from client to builder
* Build output is streamed in real time to the client
* Container image is sent from the builder to the client
* Client disconnects from queue
* Queue destroys VM

Deploy
==================
To deploy container-builder three steps are taken
* The Builder OpenStack image must be created
* The Queue OpenStack instance must be started
* The client application must be built

Due to the non-trivial complexity of provisioning Gitlab runners handle deployment through the CI system.
Following `.gitlab-ci.yml` should provide insight into the provisioning process.
