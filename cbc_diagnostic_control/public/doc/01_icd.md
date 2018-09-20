# Overview {#overview}

A component that manages the CBC serial device and CBC line discipline Kernel module.

############################
@page cbc_diagnostic_control 
############################



cbc_diagnostic_control  is invoked as follows:

    $ cbc_diagnostic_control  [options] 

The command line options are described below.

## Command line options {#cli}

Option                | Short Option | Description
--------------------- | ------------ | -----------
--help                | -h           | Prints help.
--bootloaderVersion   | -b           | Print the bootloader version.
--firmwareVersion     | -f           | Print the firmware version.
--mainboardVersion    | -m           | Print the mainboard version.
--bootTimestamps      | -t 	     | Print AIOC boot timestamps.
--logTimeStampsToFile | -l 	     | Log AIOC boot timestamps to file (default is /tmp/IOC_timestamps.txt.

