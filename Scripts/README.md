# Server Control

Prerequisite: On Titan
* module load python
* pip install --user python-openstackclient

Servers may be spun up anywhere on the ORNL network and also the Titan login nodes. To obtain the credentials required for access take the following steps.
* Login to `cloud.cades.ornl.gov`
* Navigate to `Compute -> Access & Security -> API Access`
* Click `Download OpenStack RC File v3`
* Rename downloaded file as openrc.sh and move it to `ContainerBuilder/Scripts`
* Modify `openrc.sh` to hardcode `$OS_PASSWORD_INPUT` and remove the interactive prompt
* Add the following to the bottom of `openrc.sh`
```
export OS_PROJECT_DOMAIN_NAME=$OS_USER_DOMAIN_NAME
export OS_IDENTITY_API_VERSION="3"
```

To initiate the Containerbuilder service several steps are required.
* Create the Builder master OpenStack image which will be used by the Builder Queue
* Bring up the BuilderQueue OpenStack instance

First the builder master image must be created.
```
ContainerBuilder/Scripts/CreateBuilderImage
```
After creation an SSH key will be created: `ContainerBuilderKey`

`ContainerBuilderKey` provides SSH access to the BuilderQueue as well as all of the builders


Once the builder master image has been created the queue can be safely brought up.
```
ContainerBuilder/Scripts/BringUpQueue
```

During bring up `openrc.sh` will be copied to `/home/queue` on the `BuilderQueue` host, these credentials are required to bring up builders.


`ContainerBuilder` should now be functional


To login to the queue or a builder:
```
ssh -i ./ContainerBuilderKey cades@<BuilderQueueIP/BuilderIP>
```

To destroy a new queue instance:
```
ContainerBuilder/Scripts/TearDownQueue
```

---
Note: To use the openstack client from Titan yo must export `OS_CACERT=$(pwd)/OpenStack.cer`

Note: `openstack list` can be called to show all active OpenStack instances including their ID, name, and IP

Note: To check if the BuilderQueue and Builder services are running ssh to each node and run `systemctl status`. To diagnose issues use `sudo journalctl`
