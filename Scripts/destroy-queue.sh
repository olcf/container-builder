#!/bin/bash

# Get script directory
SCRIPT_DIR=$(dirname $0)

# Delete VMs
openstack server list -f value --name BuilderQueue -c ID | while read ID; do
  echo "Deleting server ${ID}"
  openstack server delete --wait ${ID}
done