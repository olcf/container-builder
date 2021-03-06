#!/bin/bash

set -e
set -o xtrace

# Get script directory
SCRIPT_DIR=$(dirname $0)

# Delete any builders up and running
openstack server list -f value --name Builder -c ID | while read ID; do
  echo "Deleting server ${ID}"
  openstack server delete --wait ${ID}
done

# Delete master image server if exists
if [ $(openstack server list | grep "\<BuilderMaster\>" | wc -l) != 0 ]; then
    openstack server delete --wait BuilderMaster
fi

# Delete master image if it exists
if [ $(openstack image list | grep "\<BuilderImage\>" | wc -l) != 0 ]; then
    openstack image delete "BuilderImage"
fi

# Remove Key
if [ $(openstack keypair list | grep ContainerBuilderKey | wc -l) != 0 ]; then
    echo "Deleting ContainerBuilderKey"
    openstack keypair delete ContainerBuilderKey
    rm -f ${SCRIPT_DIR}/ContainerBuilderKey
fi

# Delete the security group
openstack security group list -f value | grep container_builder | while read LINE; do
  ARR_LINE=(${LINE})
  ID=${ARR_LINE[0]}
  echo "Deleting security group ${ID}"
  openstack security group delete ${ID}
done

# Remove IP files
rm -f ${SCRIPT_DIR}/BuilderIP