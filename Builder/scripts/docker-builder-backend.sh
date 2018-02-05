#!/bin/bash

# This script is run as root and so builder user environment variables aren't passed through
source /home/builder/registry-credentials
rm /home/builder/registry-credentials

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

# We don't wish to show the stdout of secondary commands unless debugging is enabled
if [[ -z ${DEBUG_FLAG+x} ]]; then
  exec {OUT_FD}>/dev/null
else
  OUT_FD=1
fi

# Provide read only access to the private gitlab docker repository if using the container-recipes docker registry
grep 'FROM code.ornl.gov:4567' ./container.def
GREP_RC=$?
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using OLCF Gitlab registry login credentials"
    /usr/local/bindocker login code.ornl.gov:4567 -u ${GITLAB_READONLY_USERNAME} -p ${GITLAB_READONLY_TOKEN} 2>&1 >&${OUT_FD}
fi

# provide read only access to the private olcf dockerhub repository
grep 'FROM olcf/' ./container.def
GREP_RC=$?
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using OLCF Dockerhub registry login credentials"
    /usr/local/bin/docker login -u ${DOCKERHUB_READONLY_USERNAME} -p ${DOCKERHUB_READONLY_TOKEN} 2>&1 >&${OUT_FD}
fi

# Spin up local registry
/usr/local/bin/docker ${DEBUG_FLAG} run -d -p 5000:5000 --restart=always --name registry registry:2 2>&1 >&${OUT_FD}

# Build the Dockerfile docker image in the current directory
mv ./container.def Dockerfile
/usr/bin/unbuffer /usr/local/bin/docker ${DEBUG_FLAG} build -t localhost:5000/docker_image:latest . || { echo 'Build Failed' ; exit 1; }

# Push to the local registry
/usr/local/bin/docker ${DEBUG_FLAG} push localhost:5000/docker_image:latest

# Build the singularity container from the docker image
export SINGULARITY_CACHEDIR=/home/builder/.singularity
export SINGULARITY_NOHTTPS=true
export SINGULARITY_PULLFOLDER=/home/builder
/usr/bin/unbuffer /usr/local/bin/singularity ${DEBUG_FLAG} pull --name container.simg docker://localhost:5000/docker_image:latest

# Workaround for PULLFOLDER not being respected: https://github.com/singularityware/singularity/pull/855
if [ -e ${SINGULARITY_CACHEDIR}/container.simg ] ; then
    mv ${SINGULARITY_CACHEDIR}/container.simg ./container.simg
fi