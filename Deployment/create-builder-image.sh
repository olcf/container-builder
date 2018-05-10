#!/bin/bash

set -e
set -x

# OpenStack credentials will be sourced by the gitlab runners

# Get script directory
SCRIPT_DIR=$(dirname $0)

# Create ContainerBuilder security group allowing tcp access to port 5000, 8080 and 22
openstack security group create container_builder --description "Allow ContainerBuilder communication"
openstack security group rule create container_builder --protocol tcp --dst-port 22:22 --remote-ip 0.0.0.0/0
openstack security group rule create container_builder --protocol tcp --dst-port 8080:8080 --remote-ip 0.0.0.0/0

# Create Keys for cades user
KEY="ContainerBuilderKey"
KEY_FILE="${SCRIPT_DIR}/artifacts/${KEY}"

# Create a new keypair named ContainerBuilderKey
openstack keypair create ${KEY} > ${KEY_FILE}
chmod 600 ${KEY_FILE}

# General VM settings
BOOTIMG="CADES_Ubuntu16.04_v20180124_1"
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
    sleep 3
done

# Reboot to fix a strange issue with apt-get update:
# "could not get lock /var/lib/apt/lists/lock - open (11: Resource temporarily unavailable)"
echo "Reboot the server to work around /var/lib/apt/lists/lock issue when using apt"
openstack server reboot --wait ${VM_UUID}
sleep 10
while ! ssh_is_up; do
    sleep 3
done

echo "Fixing ORNL TCP timeout issue for current session"
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo bash -s' < ${SCRIPT_DIR}/disable-TCP-timestamps.sh

echo "Provisioning the builder"
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo bash -s' < ${SCRIPT_DIR}/provision-builder.sh

# Copy readonly credentials to the builder, these variables must be set in
# the gitlab runner that's running this script
echo "GITLAB_READONLY_USERNAME=${GITLAB_READONLY_USERNAME}" > ./environment.sh
echo "GITLAB_READONLY_TOKEN=${GITLAB_READONLY_TOKEN}" >> ./environment.sh
echo "DOCKERHUB_READONLY_USERNAME=${DOCKERHUB_READONLY_USERNAME}" >> ./environment.sh
echo "DOCKERHUB_READONLY_TOKEN=${DOCKERHUB_READONLY_TOKEN}" >> ./environment.sh
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib" >> ./environment.sh
scp -o StrictHostKeyChecking=no -i ${KEY_FILE} ./environment.sh cades@${VM_IP}:/home/cades/environment.sh
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo mv /home/cades/environment.sh /home/builder/environment.sh'
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo chown builder /home/builder/environment.sh'

echo "Rebooting the server to ensure a clean state before creating the snapshot"
openstack server reboot --wait ${VM_UUID}

echo "Shutting down server"
openstack server stop ${VM_UUID}
until openstack server list --status SHUTOFF | grep ${VM_UUID} > /dev/null 2>&1; do
  sleep 3
done
echo -ne "\n"

echo "Sleeping for (1) minute to make sure builder is completely shut down"
sleep 60

echo "Creating builder snapshot image"
openstack server image create --wait --name BuilderImage ${VM_UUID}

echo "Deleting builder master instance"
openstack server delete --wait ${VM_UUID}

echo "Finished creating BuilderImage"
