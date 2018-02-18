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
    docker login code.ornl.gov:4567 -u ${GITLAB_READONLY_USERNAME} -p ${GITLAB_READONLY_TOKEN} 2>&${OUT_FD} 1>&${OUT_FD}
fi

# provide read only access to the private olcf dockerhub repository
grep 'FROM olcf/' ./container.def
GREP_RC=$?
if [[ ${GREP_RC} -eq 0 ]] ; then
    echo "Using OLCF Dockerhub registry login credentials"
    docker login -u ${DOCKERHUB_READONLY_USERNAME} -p ${DOCKERHUB_READONLY_TOKEN} 2>&${OUT_FD} 1>&${OUT_FD}
fi

# Spin up local registry
docker ${DEBUG_FLAG} run -d -p 5000:5000 --restart=always --name registry registry:2 2>&${OUT_FD} 1>&${OUT_FD}

# Build the Dockerfile docker image in the current directory
mv ./container.def Dockerfile
${TTY} /usr/bin/docker ${DEBUG_FLAG} build -t localhost:5000/docker_image:latest . || { echo 'Build Failed' ; exit 1; }

# Push to the local registry
${TTY} /usr/bin/docker ${DEBUG_FLAG} push localhost:5000/docker_image:latest

# Build the singularity container from the docker image
export SINGULARITY_CACHEDIR=/home/builder/.singularity
export SINGULARITY_NOHTTPS=true # Needed as we're pulling from localhost
export SINGULARITY_PULLFOLDER=/home/builder
${TTY} /usr/local/bin/singularity ${DEBUG_FLAG} build container.simg docker://localhost:5000/docker_image:latest