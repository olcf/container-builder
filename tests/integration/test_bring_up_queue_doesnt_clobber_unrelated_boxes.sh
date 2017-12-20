#!/bin/bash -e
BOOTIMG="CADES_Ubuntu16.04_v20170804_1"
ZONE="nova"
FLAVOR="m1.medium"
NIC=$(openstack network show -c id --format value or_provider_general_extnetwork1)

echo "# Startup dummy VM"
DUMMY_VM=$(openstack server create                                       \
    --image "${BOOTIMG}"                                                \
    --flavor "${FLAVOR}"                                                \
    --availability-zone "${ZONE}"                                       \
    --nic net-id="${NIC}"                                               \
    --wait                                                              \
    -c id                                                               \
    -f value                                                            \
    "DummyVM");

echo "# Tear down queue"
cd Scripts
./TearDownQueue

echo "# Assert that dummy VM was unaffected"
openstack server list | grep ${DUMMY_VM}

echo "# Cleanup dummy VM"
openstack server delete --wait ${DUMMY_VM}
