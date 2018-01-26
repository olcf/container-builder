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

# Provide read only access to the private gitlab docker repository if using the container-recipes docker registry
grep 'code.ornl.gov:4567' ./container.def
GREP_RC=$?
if [[ $GREP_RC -eq 0 ]] ; then
    echo "Using container recipes docker registry login credentials"
    docker ${DEBUG_FLAG} login code.ornl.gov:4567 -u $(cat /home/builder/gitlab-username) -p $(cat /home/builder/gitlab-readonly-token)
fi

# provide read only access to the private olcf dockerhub repository
grep 'FROM olcf/' ./container.def
GREP_RC=$?
if [[ $GREP_RC -eq 0 ]] ; then
    echo "Using OLCF Dockerhub registry login credentials"
    docker ${DEBUG_FLAG} login code.ornl.gov:4567 -u $(cat /home/builder/dockerhub-readonly-username) -p $(cat /home/builder/dockerhub-readonly-password)
fi

# Spin up local registry
docker ${DEBUG_FLAG} run -d -p 5000:5000 --restart=always --name registry registry:2

# Build the Dockerfile docker image in the current directory
mv ./container.def Dockerfile
docker ${DEBUG_FLAG} build -t localhost:5000/docker_image .

# Push to the local registry
docker ${DEBUG_FLAG} push localhost:5000/docker_image

# Build the singularity container from the docker image
singularity ${DEBUG_FLAG} pull --name container.simg docker://localhost:5000/docker_image