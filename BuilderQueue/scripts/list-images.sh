#!/bin/bash

/usr/local/bin/docker-ls tags --progress-indicator=false --user ${DOCKERHUB_READONLY_USERNAME} --password ${DOCKERHUB_READONLY_TOKEN} olcf/titan

/usr/local/bin/docker-ls tags --progress-indicator=false --user ${DOCKERHUB_READONLY_USERNAME} --password ${DOCKERHUB_READONLY_TOKEN} olcf/summitdev

/usr/local/bin/docker-ls tags --progress-indicator=false --user ${DOCKERHUB_READONLY_USERNAME} --password ${DOCKERHUB_READONLY_TOKEN} olcf/summit