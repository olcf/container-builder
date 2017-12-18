#!/usr/bin/env bash

module load python/3.5.1
pyvenv .venv
source .venv/bin/activate
pip install python-openstackclient

cd Scripts
./CreateBuilderImage
./BringUpQueue