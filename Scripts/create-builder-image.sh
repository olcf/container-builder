#!/bin/bash

set -e
set -o xtrace

export OS_CACERT=`pwd`/OpenStack.cer
echo "using OS_CACERT="${OS_CACERT}

# OpenStack credentials will be sourced by the gitlab runners

# Destroy any existing builder if one exists
./tear-down-queue.sh --no_source
./destroy-builder-image.sh --no_source

# Get script directory
SCRIPT_DIR=$(dirname $0)

# Create ContainerBuilder security group allowing tcp access to port 5000, 8080 and 22
openstack security group create container_builder --description "Allow ContainerBuilder communication"
openstack security group rule create container_builder --protocol tcp --dst-port 22:22 --remote-ip 0.0.0.0/0
openstack security group rule create container_builder --protocol tcp --dst-port 8080:8080 --remote-ip 0.0.0.0/0

# Create Keys for cades user
KEY="ContainerBuilderKey"
KEY_FILE="${SCRIPT_DIR}/../artifacts/${KEY}"

# Create a new keypair named ContainerBuilderKey
openstack keypair create ${KEY} > ${KEY_FILE}
chmod 600 ${KEY_FILE}

# General VM settings
BOOTIMG="CADES_Ubuntu16.04_v20170804_1"
ZONE="nova"
FLAVOR="m1.large"
NIC=$(openstack network show -c id --format value or_provider_general_extnetwork1)
KEY="ContainerBuilderKey"
echo "Bringing up VM"

# Startup new VM
VM_UUID=$(openstack server create                        \
    --image "${BOOTIMG}"                                 \
    --flavor "${FLAVOR}"                                 \
    --availability-zone "${ZONE}"                        \
    --nic net-id="${NIC}"                                \
    --key-name "${KEY}"                                  \
    --wait                                               \
    -c id                                                \
    -f value                                             \
    "BuilderMaster");

VM_IP=$(openstack server show -c addresses --format value ${VM_UUID} | sed -e "s/^or_provider_general_extnetwork1=//")

echo "Waiting for SSH to come up"
function ssh_is_up() {
    ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} exit &> /dev/null
}
while ! ssh_is_up; do
    sleep 1
done

echo "Fixing ORNL TCP timeout issue for current session"
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo bash -s' < ${SCRIPT_DIR}/disable-TCP-timestamps.sh

echo "Provisioning the builder"
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo bash -s' < ${SCRIPT_DIR}/provision-builder.sh

# Copy Gitlab docker registry access token to VM and then move to correct directory
# This credentials are available as environment variables to the runners
echo ${GL_TOKEN} > ${SCRIPT_DIR}/container-registry-token
scp -o StrictHostKeyChecking=no -i ${KEY_FILE} ${SCRIPT_DIR}/container-registry-token cades@${VM_IP}:/home/cades/container-registry-token
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo mv /home/cades/container-registry-token /home/builder/container-registry-token'

echo "Reboot the server to ensure its in a clean state before creating the snapshot"
openstack server reboot --wait ${VM_UUID}

echo "Shutting down server"
openstack server stop ${VM_UUID}
until openstack server list --status SHUTOFF | grep ${VM_UUID} > /dev/null 2>&1; do
  sleep 1
done
echo -ne "\n"

echo "Sleeping for a bit to make sure builder is completely shut down"
sleep 60

echo "Creating builder snapshot image"
openstack server image create --wait --name BuilderImage ${VM_UUID}

echo "Deleting builder master instance"
openstack server delete --wait ${VM_UUID}

echo "Finished creating BuilderImage"