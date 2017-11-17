#!/bin/bash

losetup -f
sudo /usr/local/bin/singularity build ./container.img ./container.def
