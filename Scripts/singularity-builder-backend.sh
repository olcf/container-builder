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

# Provide readonly access to the private gitlab docker repository if using the container-recipes docker registry
grep 'code.ornl.gov:4567' ./container.def
GREP_RC=$?
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using container recipes docker registry login credentials"
    export SINGULARITY_DOCKER_USERNAME=atj
    export SINGULARITY_DOCKER_PASSWORD=$(cat /home/builder/container-registry-token)
fi

/usr/bin/unbuffer /usr/local/bin/singularity ${DEBUG_FLAG} build ./container.simg ./container.def