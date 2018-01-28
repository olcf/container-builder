#!/bin/bash

set -e
set -o xtrace

mkdir artifacts

cd Scripts
./create-builder-image.sh
./bring-up-queue.sh