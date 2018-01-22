#!/bin/bash

set -e
set -o xtrace

export OS_CACERT=`pwd`/OpenStack.cer
echo "using OS_CACERT="${OS_CACERT}

# Get script directory
SCRIPT_DIR=$(pwd)

# General VM settings
BOOTIMG="CADES_Ubuntu16.04_v20170804_1"
ZONE="nova"
FLAVOR="m1.large"
NIC=$(openstack network show -c id --format value or_provider_general_extnetwork1)

KEY="ContainerBuilderKey"
KEY_FILE="${SCRIPT_DIR}/../artifacts/${KEY}"

echo "Bringing up VM"

# Startup new VM
VM_UUID=$(openstack server create                                       \
    --image "${BOOTIMG}"                                                \
    --flavor "${FLAVOR}"                                                \
    --availability-zone "${ZONE}"                                       \
    --nic net-id="${NIC}"                                               \
    --key-name "${KEY}"                                                 \
    --wait                                                              \
    -c id                                                               \
    --security-group container_builder                                  \
    -f value                                                            \
    "BuilderQueue");

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

echo "Provisioning the queue"
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo bash -s' < ${SCRIPT_DIR}/provision-queue.sh

# Copy OpenStack credentials to VM and then move to correct directory
# These credentials are available as environment variables to the runners
unset OS_CACERT
printenv | grep ^OS_ > ${SCRIPT_DIR}/openrc.sh # "Reconstruct" openrc.sh
awk '{print "export "$0}' ${SCRIPT_DIR}/openrc.sh > tmp_awk && mv tmp_awk ${SCRIPT_DIR}/openrc.sh
scp -o StrictHostKeyChecking=no -i ${KEY_FILE} ${SCRIPT_DIR}/openrc.sh cades@${VM_IP}:/home/cades/openrc.sh
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo mv /home/cades/openrc.sh /home/queue/openrc.sh'

# Reboot to ensure Queue service, added in provisioning, is started
export OS_CACERT=`pwd`/OpenStack.cer
openstack server reboot --wait ${VM_UUID}

echo "Started ${VM_UUID} with external IP ${VM_IP} using ${KEY_FILE}"

cat << EOF > ${SCRIPT_DIR}/../artifacts/queue-host.sh
QUEUE_HOST=${VM_IP}
EOF