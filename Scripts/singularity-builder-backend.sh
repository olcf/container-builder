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
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using container recipes docker registry login credentials"
    export SINGULARITY_DOCKER_USERNAME=$(cat /home/builder-gitlab-username)
    export SINGULARITY_DOCKER_PASSWORD=$(cat /home/builder/gitlab-readonly-token)
fi

# provide read only access to the private olcf dockerhub repository
grep 'FROM olcf/' ./container.def
GREP_RC=$?
if [[ $GREP_RC -eq 0 ]] ; then
    echo "Using OLCF Dockerhub registry login credentials"
    export SINGULARITY_DOCKER_USERNAME=$(cat /home/builder/dockerhub-readonly-username)
    export SINGULARITY_DOCKER_PASSWORD=$(cat /home/builder/dockerhub-readonly-token)
fi

/usr/bin/unbuffer /usr/local/bin/singularity ${DEBUG_FLAG} build ./container.simg ./container.def