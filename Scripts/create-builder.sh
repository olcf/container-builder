#!/bin/bash

# OpenStack credentials
source /home/queue/openrc.sh

# General VM settings
BOOTIMG="BuilderImage"
ZONE="nova"
FLAVOR="m1.large"
KEY="ContainerBuilderKey"

NIC=$(openstack network show -c id --format value or_provider_general_extnetwork1)

# Startup new VM
openstack server create               \
    --image "${BOOTIMG}"              \
    --flavor "${FLAVOR}"              \
    --availability-zone "${ZONE}"     \
    --nic net-id="${NIC}"             \
    --key-name "${KEY}"               \
    --security-group container_builder\
    -f json                           \
    --wait                            \
    "Builder"