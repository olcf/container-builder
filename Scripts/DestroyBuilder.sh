#!/bin/bash

set -e

# OpenStack credentials
source /home/queue/openrc.sh

openstack server delete --wait $1