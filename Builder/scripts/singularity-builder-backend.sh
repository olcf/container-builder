#!/bin/bash

source /home/builder/environment.sh

# Test for any arguments
for i in "$@"
do
case ${i} in
    --debug)
    DEBUG_FLAG='--debug'
    shift
    ;;
    --tty)
    TTY='/usr/bin/unbuffer'
    shift
    ;;
    *)
      echo "unknown argument to singularity-builder-backend.sh"
      exit 1
    ;;
esac
done

# Provide read only access to the private gitlab docker repository if using the container-recipes docker registry
grep 'From: code.ornl.gov:4567' ./container.def
GREP_RC=$?
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using OLCF Gitlab registry login credentials"
    export SINGULARITY_DOCKER_USERNAME=${GITLAB_READONLY_USERNAME}
    export SINGULARITY_DOCKER_PASSWORD=${GITLAB_READONLY_TOKEN}
fi

# provide read only access to the private olcf dockerhub repository
grep 'From: olcf/' ./container.def
GREP_RC=$?
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using OLCF Dockerhub registry login credentials"
    export SINGULARITY_DOCKER_USERNAME=${DOCKERHUB_READONLY_USERNAME}
    export SINGULARITY_DOCKER_PASSWORD=${DOCKERHUB_READONLY_TOKEN}
fi

export SINGULARITY_CACHEDIR=/home/builder/.singularity
${TTY} /usr/local/bin/singularity ${DEBUG_FLAG} build ./container.simg ./container.def