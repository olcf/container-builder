#!/bin/bash

# Get script directory
SCRIPT_DIR=$(dirname $0)

# OpenStack credentials
if [ "$1" != "--no_source" ]; then
    source ${SCRIPT_DIR}/openrc.sh
fi

# Delete VMs
openstack server list -f value --name BuilderQueue -c ID | while read ID; do
  echo "Deleting server ${ID}"
  openstack server delete --wait ${ID}
done