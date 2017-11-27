# Server Control

Prerequisite: On Titan
* module load python
* pip install --user python-openstackclient

To Begin download an OpenStack RC file onto a system on the ORNL network, or Titan
* Login to `cloud.cades.ornl.gov`
* Navigate to `Compute -> Access & Security -> API Access`
* Click `Download OpenStack RC File v3`
* Rename downloaded file as openrc.sh and move it to `SingularityTools/Builder/Server/BuilderControl`
* Modify `openrc.sh` to hardcode `$OS_PASSWORD_INPUT` and remove the interactive prompt
* Add the following to the bottom of `openrc.sh`
```
export OS_PROJECT_DOMAIN_NAME=$OS_USER_DOMAIN_NAME
export OS_IDENTITY_API_VERSION="3"
```

To prime the Containerbuilder service several steps are required
* Bring up the BuilderQueue OpenStack instance
* Create the Builder master OpenStack image which will be used to quickly spin up builders

To bring up a new builder instance:
```
ContainerBuilder/Scripts/BringUpQueue
```
After bringup an SSH key will be created: `ContainerBuilderKey`

 `ContainerBuilderKey` provides SSH access to the BuilderQueue as well as all of the builders

During bring up `openrc.sh` will be copied to `/home/queue`, these credentials are required to bring up builders.

After the Queue has been brought up the master builder image must be created:
```
ContainerBuilder/Scripts/CreateBuilderImage
```


To login to the queue or a builder:
```
ssh -i ./ContainerBuilder cades@`BuilderQueueIP/Builder`
```

To destroy a new queue instance:
```
ContainerBuilder/Scripts/TearDownQueue
```

Note: Titan doesn't have the SSL certs in place and so before running any nova commands `OS_CACERT` must be set.

Note: `openstack list` can be called to show all active OpenStack instances including their ID, name, and IP

Note: To check if the BuilderQueue and Builder services are running ssh to each node and run `systemctl status`. To diagnose issues use `sudo journalctl`