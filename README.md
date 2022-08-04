DISCONTINUATION OF PROJECT.

This project will no longer be maintained by Intel.

Intel has ceased development and contributions including, but not limited to, maintenance, bug fixes, new releases, or updates, to this project. 

Intel no longer accepts patches to this project.

If you have an ongoing need to use this project, are interested in independently developing it, or would like to maintain patches for the open source software community, please create your own fork of this project. 
# ioc-cbc-tools

IOC stands for IO controller which is used for automotive system.
CBC stands for carrier board communiction protocol.

This package will provide the userspace systemd services to:
1. attach CBC Ldisc to a certain tty port
2. bind to /dev/cbc-lifecycle to provide lifecycle support for OS.
3. bind to /dev/diagnosis to provide thermal sensor data and fan control.
4. ioc diagnosis and logging service

The package could work on Native Linux, Guest Linux and also the
Service OS Linux environment supported by ACRN.

For all security issues please see https://01.org/security.
