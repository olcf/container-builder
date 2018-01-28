#!/bin/bash

set -e
set -o xtrace

pyvenv .venv
source .venv/bin/activate
pip install python-openstackclient

mkdir artifacts

cd Scripts
./create-builder-image.sh
./bring-up-queue.sh