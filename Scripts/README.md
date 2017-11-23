# Server Control

Prerequisite: On Titan
* module load python
* pip install --user python-novaclient
* pip install --user python-glanceclient

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
After bringup two files will be created:
 * ResourceQueueIP
 * ContainerBuilderKey

 `ContainerBuilderKey` provides SSH access to the ResourceQueue as well as all of the builders

During bring up `openrc.sh` will be copied to `/home/queue`, these credentials are required to bring up builder resources.

After the Queue has been brought up the master builder image must be created:
```
ContainerBuilder/Scripts/CreateBuilderImage
```


To login to the queue
```
ssh -i ./ResourceQueueKey cades@`cat ResourceQueueIP`
```

To destroy a new queue instance:
```
ContainerBuilder/Scripts/TearDownQueue
```

Note: Titan doesn't have the SSL certs in place and so before running any nova commands `OS_CACERT` must be set. For glance this env variable is ignored and you must used `glance --ca-file=`pwd`/OpenStack.cer ...`
Note: `nova list` can be called to show all active OpenStack instances
