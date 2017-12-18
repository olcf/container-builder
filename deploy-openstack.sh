#!/usr/bin/env bash

module load python/3.5.1
pyvenv .venv
source .venv/bin/activate
pip install python-openstackclient

cd Scripts
./CreateBuilderImage
./BringUpQueue

cat << EOF > queue_host.sh
QUEUE_HOST=12345
EOF