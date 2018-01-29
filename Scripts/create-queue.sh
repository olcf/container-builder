#!/bin/bash

set -e
set -o xtrace

# Get script directory
SCRIPT_DIR=$(pwd)

# General VM settings
BOOTIMG="CADES_Ubuntu16.04_v20180124_1"
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
printenv | grep ^OS_ > ./queue_profile # "Reconstruct" openrc.sh
awk '{print "export "$0}' ./queue_profile > tmp_awk && mv tmp_awk ./queue_profile
scp -o StrictHostKeyChecking=no -i ${KEY_FILE} ./queue_profile cades@${VM_IP}:/home/cades/queue_profile
ssh -o StrictHostKeyChecking=no -i ${KEY_FILE} cades@${VM_IP} 'sudo mv /home/cades/queue_profile /home/queue/.profile'

# Reboot to ensure Queue service, added in provisioning, is started
openstack server reboot --wait ${VM_UUID}

echo "Started ${VM_UUID} with external IP ${VM_IP} using ${KEY_FILE}"

# Provide git user information required for commit
git config --global user.email "${GITLAB_ADMIN_USERNAME}@ornl.gov"
git config --global user.name ${GITLAB_ADMIN_USERNAME}

# Create queue-host file containing IP to the queue
cat << EOF > ${SCRIPT_DIR}/../queue-host
${VM_IP}
EOF

# Push changes upstream
git checkout -B master origin/master
git add ${SCRIPT_DIR}/../queue-host
git commit -m "Updating queue host IP"
git push https://${GITLAB_ADMIN_USERNAME}:${GITLAB_ADMIN_TOKEN}@code.ornl.gov/olcf/container-builder master