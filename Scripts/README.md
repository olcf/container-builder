# Server Control

To Begin download an OpenStack RC file onto a system on the ORNL network and Titan
* Login to `cloud.cades.ornl.gov`
* Navigate to `Compute -> Access & Security -> API Access`
* Click `Download OpenStack RC File v3`
* Rename downloaded file as openrc.sh and move it to `SingularityTools/Builder/Server/BuilderControl`
* Modify openrc.sh to hardcode `$OS_PASSWORD_INPUT`

To bring up a new builder instance:
```
ContainerBuilder/Scripts/BringUpQueue
```
After bringup two files will be created containing the ResourceQueue IP address as well as an SSH keys. `ContainerBuilder` provides access for the cades user for administration of the ResourceQueue and all Builders.

During bring up `openrc.sh` will be copied to `/home/queue`, these credentials are required to bring up builder resources.

After the Queue has been brought up the master builder image must be created:
```
ContainerBuilder/Scripts/CreateBuilderImage
```

----
To login to the queue
```
ssh -i ./ResourceQueueKey cades@`cat ResourceQueueIP`
```

To destroy a new queue instance:
```
ContainerBuilder/Scripts/TearDownQueue
```
