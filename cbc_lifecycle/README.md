# IOC Life Cycle

## Introduction
The IOC life cycle service manages the state machine transition flow, including reboot, suspend, resume and shutdown.

## Clearlinux Native Support / LaaG Support
```
                    .     LifeCycle Service
                    .
                    .                        |-reboot
                    .                        V
 +-----------+      .          +-------------+
 |           |<-----reboot-----|  reboot     |
 |           |      .          +-------------+
 |           |      .   
 |           |      .          +-------------+  
 |    IOC    |--ignition key-->|  suspend    |
 |           |<-----suspend----+-------------+
 |           |      .   
 |           |      .          +-------------+
 |           |---alive event-->|  resume     |
 |           |      .          +-------------+
 |           |      .                       
 |           |      .                        |-poweroff
 |           |      .                        V
 |           |      .          +-------------+
 |           |<-----shutdown---|  shutdown   |
 |           |      .          +-------------+
 +-----------+      .
```
### Implementation
For suspend/resume support, the life cycle service polls the ignition key event from IOC and kicks off the action with below code:
```
	system("echo mem > /sys/power/state");
```
For reboot/shutdown support, the life cycle service checks the system run level during service terminating and set corresponding power state in IOC HW.

## Clearlinux SOS Support
For SOS, life cycle service needs to coordinate with ACRN to apply the state change.
### IOC Mediator
IOC mediator is the virtual IOC simulated by ACRN DM. Both SOS and UOS can access IOC HW
through this virtual device.

For UOS, IOC mediator is compatible with real IOC HW.

For SOS, ACRN inserts suspend and shutdown events in IOC mediator to inform UOS VM state change.
```
  +---------+                 +---------+
  |IOC      | VM stop         |VM       |
  |Mediator |<----------------+Manager  |
  |         |                 |         |
  |         | VM suspend      |         |
  |         |<----------------+         |
  |         |                 |         |
  |         | VM resume       |         |
  |         |<----------------+         |
  |         |get_wakeup_reason|         |
  |         |for resume flow  |         |
  |         +---------------->|         |
  +---------+                 +---------+
```

The SOS life cycle service read the events from IOC mediator and sync state change with acrnd before taking action.
```
                    .                           S O S                                .
                    .                                                                .
                    .     LifeCycle Service        . ACRN manager daemon Service     .
                    .                              .                                 .
                    .                              .                                 .
                    .                              .                                 .
 +-----------+      .          +-------------+     .                 +-----------+   .
 |           |<----reboot------|  reboot     |<----sos_lcs:REBOOT----|           |   .
 |           |      .          +-------------+     .                 |           |   .
 |           |      .                              .                 |           |   .
 |           |--ignition key/->+-------------+     .                 |           |   .
 |    IOC    |  UOS VM event   |  suspend    |---acrnd:ACRND_STOP--->|   acrnd   |   .
 | Mediator  |      .          |             |<--sos_lcs:SUSPEND-----|           |   .   +------+
 |           |<----suspend-----+-------------+     .                 |           |<----->| UOS  |
 |           |      .                              .                 |           |   .   +------+
 |           |      .          +-------------+     .                 |           |   .
 |           |------alive----->|  resume     |--acrnd:ACRND_RESUME-->|           |   .
 |           |      .          +-------------+     .                 |           |   .
 |           |      .                              .                 |           |   .
 |           |--ignition key/->+-------------+     .                 |           |   .
 |           |  UOS VM event   |  shutdown   |---acrnd:ACRND_STOP--->|           |   .
 |           |      .          |             |<--sos_lcs:SHUTDOWN----|           |   .
 |           |<----shutdown----+-------------+     .                 |           |   .
 +-----------+      .                              .                 +-----------+   .
```
### Implementation
The life cycle service polls the ignition key/UOS VM events from IOC and sends ACRND_STOP to acrnd. Then, it will wait for the return event from acrnd and takes action based on acrnd request.

To send ACRND_STOP to acrnd:
```
#include <acrn/acrn_mngr.h>

static char acrnd_name[] = "acrnd";
static int send_acrnd_stop(void)
{
	int acrnd_fd;
	int ret;
	struct mngr_msg req = {
		.msgid = ACRND_STOP,
		.magic = MNGR_MSG_MAGIC,
		.data = {
			.acrnd_stop = {
				.force = 0,
				.timeout = 20,
				},
			},
	};
	struct mngr_msg ack;

	req.timestamp = time(NULL);
	acrnd_fd = mngr_open_un(acrnd_name, MNGR_CLIENT);
	if (acrnd_fd < 0) {
		fprintf(stderr, "cannot open %s socket\n", acrnd_name);
		return -1;
	}
	ret = mngr_send_msg(acrnd_fd, &req, &ack, 2);
	if (ret > 0)
		fprintf(stderr, "result %d\n", ack.data.err);
	mngr_close(acrnd_fd);
	return ret;
}
```

To listen acrnd event:
```
#include <acrn/acrn_mngr.h>

static char cbcd_name[] = "sos-lcs";
cbcd_fd = mngr_open_un(cbcd_name, MNGR_SERVER);
mngr_add_handler(cbcd_fd, SHUTDOWN, handle_shutdown, NULL);
mngr_add_handler(cbcd_fd, SUSPEND, handle_suspend, NULL);
mngr_add_handler(cbcd_fd, REBOOT, handle_reboot, NULL);

static void handle_xxx(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg ack;

	ack.magic = MNGR_MSG_MAGIC;
	ack.msgid = msg->msgid;
	ack.timestamp = msg->timestamp;
	ack.data.err = 0;
	mngr_send_msg(client_fd, &ack, NULL, 0);

	//Take action below
	... ...
}
```
## Remove IOC life cycle from clearlinux image
Create mixer workspace based on clearlinux release note and edit software-defined-cockpit with below commands:
```
mixer bundle add software-defined-cockpit
mixer bundle edit software-defined-cockpit (remove “ioc-cbc-tools”)
```
Build the update and image for clearlinux SOS/Native/UOS based on clearlinux release note.

After removing this service, suspend to RAM and reboot command will not work.
