#!/bin/bash

set -e
set -o xtrace

module load python/3.5.1
pyvenv .venv
source .venv/bin/activate
pip install python-openstackclient

mkdir artifacts

cd Scripts
./create-builder-image.sh
./bring-up-queue.sh