#!/bin/bash

# Test for any arguments, such as --debug
for i in "$@"
do
case ${i} in
    --debug)
    DEBUG_FLAG='--debug'
    shift # past argument with no value
    ;;
    *)
      echo "unknown argument to singularity-builder-backend.sh"
      exit 1
    ;;
esac
done

# Provide read-only access to gitlab registry and dockerhub
docker ${DEBUG_FLAG} login code.ornl.gov:4567 -u ${GITLAB_READONLY_USERNAME} -p ${GITLAB_READONLY_TOKEN}
docker ${DEBUG_FLAG} login code.ornl.gov:4567 -u ${DOCKERHUB_READONLY_USERNAME} -p ${DOCKERHUB_READONLY_TOKEN}

# Spin up local registry
docker ${DEBUG_FLAG} run -d -p 5000:5000 --restart=always --name registry registry:2

# Build the Dockerfile docker image in the current directory
mv ./container.def Dockerfile
docker ${DEBUG_FLAG} build -t localhost:5000/docker_image .

# Push to the local registry
docker ${DEBUG_FLAG} push localhost:5000/docker_image

# Build the singularity container from the docker image
singularity ${DEBUG_FLAG} pull --name container.simg docker://localhost:5000/docker_image