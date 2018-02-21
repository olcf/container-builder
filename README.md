container-builder
================
Containers traditionally require root access to build from a recipe file, as such native access to build cannot be granted on OLCF HPC resources such as Titan and Summit. 
container-builder is an interactive container building utility that gets around this limitation by building each container on a remote ephemeral VM, streaming the output in real time to the client.

Use
=================
`container-builder` requires two arguments, the name of the created container and the container definition.
```
$ module load container-builder
$ container-builder container.img container.recipe
... Output streams...
```

To list base images available within `container-builder` the `cb-images` command can be used
```
$ cb-images
Connecting to BuilderQueue: Connected to queue: 128.219.187.52
Requesting image list: Fetched
-----------------------------
repository: olcf/titan
tags:
- centos-7_2018-01-18
- ubuntu-16.04_2018-01-18
-----------------------------
repository: olcf/summit
tags:
- centos-7_2018-02-08
```

To get an overview of the builder VM status `cb-status` is available
```
$ cb-status
Connecting to BuilderQueue: Connected to queue: 128.219.187.52
[INFO] Requesting queue status
-------------
Active builders
-------------

ID: e6a0cd58-255c-4f17-b0df-51c5b0120d07
HOST: 128.219.187.95
PORT: 8080
-------------

-------------
Reserve builders
-------------

ID: 60c95481-92f3-4491-94d3-14c45bfbc279
HOST: 128.219.187.96
PORT: 8080
-------------

...
```  

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
