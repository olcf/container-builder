#!/bin/bash

set -e

set -o xtrace

mkdir artifacts

./destroy-queue.sh
./destroy-builder-image.sh
./create-builder-image.sh
./create-queue.sh