/*
 * Copyright (C) 2018 Intel Corporation
 *
 * Author: Alek Du <alek.du@intel.com>
 *
 * SPDX-License-identifier: BSD-3-Clause
 */

/* CBC lifecycle state machine transition flow
 *
 *                  .-------------------------------------------
 *     -------------+--------------                            |
 *     |     IOC    V     IOC     |                            |
 * (default) ==> (Active) ==> (shutdown) ==> (shutdown delay) (Off)
 *                  |_____________|__________________|
 *         _________|________   (ACRN select)                  ^
 *    ACRN/     ACRN|    ACRN\                                 |
 *  (reboot) (suspend) (shutdown)                              |
 *        |         |      |                                   |
 *        ------------------------------------------------------
 *
 * IOC: state transition due to cbc-lifecycle wakeup reason
 * ACRN: state transition due to ACRND IPC request
 */

/* Basic cbc-lifecycle workflow on Service OS
 * ____________________
 * | sos lifecycle    |
 * |    service       |
 * |                  |
 * |*incoming request |(Listen on server socket @ /run/acrn/sos-lcs.socket)
 * | wakeup reason    |
 * | shutdown         |
 * | reboot           |(Requests from ACRN VM manager)
 * | suspend          |
 * | RTC set wakeup   |
 * |                  |
 * |*out events       |(Send to ACRN VM manager @ /run/acrn/acrnd.socket)
 * | vm start         |
 * | vm stop          |(Events from /dev/cbc-lifecycle port)
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 3 threads: 1. wake on /dev/cbc-lifecycle and receive wakeup reasons
 *            2. heartbeat thread to send heartbeat msg to /dev/cbc-lifecycle
 *            3. server socket handler thread to handle incoming msg
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/tty.h>
#include <acrn/acrn_mngr.h>

static char cbcd_name[] = "sos-lcs";
static char acrnd_name[] = "acrnd";
static char cbc_lifecycle_dev[] = "/dev/cbc-lifecycle";
static char cbc_match_file[] = "/usr/share/ioc-cbc-tools/cbc_match.txt";

typedef enum {
	S_DEFAULT = 0,	     /* default, not receiving any status */
	S_ALIVE = 1,	     /* receive wakeup_reason bit 0~22 not all 0 */
	S_SHUTDOWN,	     /* receive wakeup_reason bit 0~22 off 23 on */
	S_SHUTDOWN_DELAY,    /* before ACRND confirm off, sent delay to IOC */
	S_ACRND_SHUTDOWN,    /* ACRND confirm ioc can off */
	S_IOC_SHUTDOWN,      /* receiving wakeup_reason bit 0~23 off */
	S_ACRND_REBOOT,	     /* receive request from ACRND to go reboot */
	S_ACRND_SUSPEND,     /* receive request from ACRND to go S3 */
	S_MAX,
} state_machine_t;

/* state machine valid transition table */
char valid_map[S_MAX][S_MAX] = {
	{ 1, 1, 1, 0, 0, 0, 0, 0 }, /* default can go alive or shutdown */
	{ 0, 1, 1, 0, 1, 0, 1, 1 }, /* alive can go S_ACRND_* state */
	{ 0, 0, 1, 1, 1, 1, 1, 1 }, /* shutdown can go upper states */
	{ 1, 0, 0, 1, 1, 1, 1, 1 }, /* delay can go upper states, or default (shutdown refuse) */
	{ 0, 0, 0, 0, 1, 1, 0, 0 }, /* acrnd shutdown can go ioc_shutdown */
	{ 1, 0, 0, 0, 0, 1, 0, 0 }, /* ioc_shutdown can go default (s3 case) */
	{ 0, 0, 0, 0, 0, 1, 1, 0 }, /* acrnd_reboot can only go ioc_shutdown */
	{ 0, 0, 0, 0, 0, 1, 0, 1 }, /* acrnd_suspend can go ioc_shutdown */
};

const char *state_name[] = {
	"default",
	"keep_alive",
	"shutdown",
	"shutdown_delay",
	"acrnd_shutdown",
	"ioc_shutdown",
	"acrnd_reboot",
	"acrnd_suspend",
};

static state_machine_t state;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t event_sema;

typedef struct {
	uint8_t header;
	uint8_t wakeup[3];
} __attribute__((packed)) wakeup_reason_frame;

typedef pthread_t cbc_thread_t;
typedef void* (*cbc_thread_func_t)(void *arg);

/* cbc suppress heartbeat is not used so far, keep here for future use */
static char cbc_suppress_heartbeat_1min[] =	{0x04, 0x60, 0xEA, 0x00};
static char cbc_suppress_heartbeat_5min[] =	{0x04, 0xE0, 0x93, 0x04};
static char cbc_suppress_heartbeat_10min[] =	{0x04, 0xC0, 0x27, 0x09};
static char cbc_suppress_heartbeat_30min[] =	{0x04, 0x40, 0x77, 0x1B};
static char cbc_heartbeat_shutdown[] =		{0x02, 0x00, 0x01, 0x00};
static char cbc_heartbeat_reboot[] =		{0x02, 0x00, 0x02, 0x00};
/* hearbeat shutdown ignore number is not used so far for non-native case */
static char cbc_heartbeat_shutdown_ignore_1[] =	{0x02, 0x00, 0x03, 0x00};
static char cbc_heartbeat_reboot_ignore_1[] =	{0x02, 0x00, 0x04, 0x00};
static char cbc_heartbeat_shutdown_ignore_2[] =	{0x02, 0x00, 0x05, 0x00};
static char cbc_heartbeat_reboot_ignore_2[] =	{0x02, 0x00, 0x06, 0x00};
static char cbc_heartbeat_s3[] =		{0x02, 0x00, 0x07, 0x00};
static char cbc_heartbeat_active[] =		{0x02, 0x01, 0x00, 0x00};
static char cbc_heartbeat_shutdown_delay[] =	{0x02, 0x02, 0x00, 0x00};
static char cbc_heartbeat_init[] = 		{0x02, 0x03, 0x00, 0x00};
static char cbc_heartbeat_rtc[] =		{0x05, 0x00, 0x00, 0x00};
static int cbc_rtc_set;

static int cbc_lifecycle_fd = -1;

static int wakeup_reason;
static int up_wakeup_reason;

static void wait_for_device(const char *dev_name)
{
	int loop = 360; // 180 seconds
	if (dev_name == NULL)
		return;

	while (loop--) {
		if (access(dev_name, F_OK)) {
			fprintf(stderr, "waiting for %s\n", dev_name);
			usleep(500000);
			continue;
		}
		break;
	}
}

static int open_cbc_device(const char *cbc_device)
{
	int fd;
	wait_for_device(cbc_device);

	if ((fd = open(cbc_device, O_RDWR | O_NOCTTY)) < 0) {
		fprintf(stderr, "%s failed to open %s\n", __func__, cbc_device);
		return -1;
	}
	return fd;
}

static void close_cbc_device(int fd)
{
	close(fd);
}

static int cbc_send_data(int fd, const char *payload, int len)
{
	int ret = 0;

	while (1) {
		ret = write(fd, payload, len);
		if (ret <= 0) {
			if (errno == EDQUOT) {
				usleep(1000);
				break;
			} else if (errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "%s issue : %d", __func__,
						errno);
				break;
			}
		}
		break;
	}
	return ret;
}

static int cbc_read_data(int fd, char *payload, int len)
{
	int nbytes = 0;

	while (1) {
		nbytes = read(fd, payload, len);

		if (nbytes < 0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "%s issue %d", __func__, errno);
			usleep(5000);
			return 0;
		}
		break;
	}
	return nbytes;
}

static __inline__ int cbc_thread_create(cbc_thread_t *pthread,
		cbc_thread_func_t start, void *arg)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	return pthread_create(pthread, (const pthread_attr_t *)&attr,
			start, arg);
}

state_machine_t get_state(void)
{
	state_machine_t _state;
	pthread_mutex_lock(&state_mutex);
	_state = state;
	pthread_mutex_unlock(&state_mutex);
	return _state;
}

state_machine_t state_transit(state_machine_t new)
{
	state_machine_t _state;
	pthread_mutex_lock(&state_mutex);

	_state = state;
	if (valid_map[state][new]) {
		state = new;
		pthread_mutex_unlock(&state_mutex);
		if (_state != new) {
			fprintf(stderr, "transit (%s to %s)\n",
				state_name[_state], state_name[new]);
			_state = new;
		}
	} else {
		pthread_mutex_unlock(&state_mutex);
	}
	return _state;
}

static int send_acrnd_stop(void);
static int send_acrnd_start(void);

#define RETRY_CNT 5
void *cbc_heartbeat_loop(void)
{
	struct timespec ts;
	state_machine_t last_state = S_DEFAULT;
	const int p_size = sizeof(cbc_heartbeat_init);
	int start_retry = 0;
	int stop_retry = 0;
	int default_cnt = 0;

	cbc_send_data(cbc_lifecycle_fd, cbc_heartbeat_init, p_size);
	fprintf(stderr, "send heartbeat init\n");

	while (1) {
		char *heartbeat = NULL;
		state_machine_t cur_state = get_state();

		switch (cur_state) {
		case S_DEFAULT:
			if (last_state != S_DEFAULT)
				default_cnt = 0;
			if (default_cnt++) {
				heartbeat = cbc_heartbeat_init;
				fprintf(stderr, "send heartbeat init\n");
			}
			start_retry = 0;
			break;
		case S_ALIVE:
			if (last_state != S_ALIVE)
				up_wakeup_reason = wakeup_reason;
			if ((last_state != S_ALIVE) || (start_retry > 0)) {
				if (send_acrnd_start() == -1) {
					if (!start_retry)
						start_retry = RETRY_CNT;
					else
						start_retry--;
				} else {
					start_retry = 0;
				}
			}
			heartbeat = cbc_heartbeat_active;
			break;
		case S_SHUTDOWN:
			/* when ACRND detects our off request, we must wait if
			 * UOS really accept the requst, thus we send shutdown
			 * delay */
			cur_state = state_transit(S_SHUTDOWN_DELAY);
			if (cur_state != S_SHUTDOWN_DELAY)// race condition !
				break;
			if (send_acrnd_stop() == -1)
				stop_retry = RETRY_CNT;
			else
				stop_retry = 0;
			// falling through
		case S_SHUTDOWN_DELAY:
			if (stop_retry > 0) {
				if (send_acrnd_stop() == -1) {
					stop_retry--;
					if (!stop_retry) {
						fprintf(stderr, "no one handle our stop request, assume suspend\n");
						state_transit(S_ACRND_SUSPEND);
						continue;
					}
				} else {
					stop_retry = 0;
				}
			}
			heartbeat = cbc_heartbeat_shutdown_delay;
			break;
		case S_ACRND_SHUTDOWN:
			heartbeat = cbc_heartbeat_shutdown;
			break;
		case S_ACRND_REBOOT:
			heartbeat = cbc_heartbeat_reboot;
			break;
		case S_ACRND_SUSPEND:
			heartbeat = cbc_heartbeat_s3;
			break;
		case S_IOC_SHUTDOWN:
			if (last_state == S_ACRND_SHUTDOWN) {
				system("shutdown 0");
			} else if (last_state == S_ACRND_REBOOT) {
				system("reboot");
			} else if (last_state == S_ACRND_SUSPEND) {
				if (cbc_rtc_set) {
					cbc_send_data(cbc_lifecycle_fd,
						cbc_heartbeat_rtc, p_size);
					cbc_rtc_set = 0;
				}
				system("systemctl suspend");
			}
			state_transit(S_DEFAULT);// for s3 case
			up_wakeup_reason = 0; // reset up wakeup reason
			break;
		}
		if (heartbeat) {
			cbc_send_data(cbc_lifecycle_fd, heartbeat, p_size);
			fprintf(stderr, ".");
		}
		last_state = cur_state;
		clock_gettime(CLOCK_REALTIME, &ts);
		/* delay 1 second to send next heart beat */
		ts.tv_sec += 1;
		sem_timedwait(&event_sema, &ts);
	}
	return NULL;
}

static int force_s5;
void *cbc_wakeup_reason_thread(void *arg)
{
	wakeup_reason_frame data;
	int len;

	while (1) {
		len = cbc_read_data(cbc_lifecycle_fd, (uint8_t *)&data, sizeof(data));
		if (len > 0) {
			if (data.header == 6) { // TODO: handle logic mode value
				continue;
			}
			if (data.header != 1) {
				fprintf(stderr, "received wrong wakeup reason");
				continue;
			}
			wakeup_reason = data.wakeup[0] | data.wakeup[1] << 8 | data.wakeup[2] << 16;
			if (!wakeup_reason) {
				state_transit(S_IOC_SHUTDOWN);
				sem_post(&event_sema);
			} else if (!(wakeup_reason & ~(3 << 22))) {
				state_transit(S_SHUTDOWN);
				// bit 22 is used by UOC ioc mediator to indicate a S5 is preferred
				force_s5 = wakeup_reason & (1 << 22);
			} else {
				state_transit(S_ALIVE);
			}
		}
	}
	return NULL;
}

static int cbcd_fd;

static void handle_shutdown(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg ack;

	ack.magic = MNGR_MSG_MAGIC;
	ack.msgid = msg->msgid;
	ack.timestamp = msg->timestamp;
	ack.data.err = 0;

	if (msg->data.err) {
		fprintf(stderr, "acrnd refuse to shutdown\n");
		state_transit(S_DEFAULT);// wakeup reason will override this anyway
	} else {
		fprintf(stderr, "acrnd agreed to shutdown\n");
		state_transit(S_ACRND_SHUTDOWN);
	}
	sem_post(&event_sema);
	mngr_send_msg(client_fd, &ack, NULL, 0);
}

static void handle_suspend(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg ack;

	ack.magic = MNGR_MSG_MAGIC;
	ack.msgid = msg->msgid;
	ack.timestamp = msg->timestamp;
	ack.data.err = 0;

	if (msg->data.err) {
		fprintf(stderr, "acrnd refuse to suspend\n");
		state_transit(S_DEFAULT);// wakeup reason will override this anyway
	} else {
		fprintf(stderr, "acrnd agreed to suspend\n");
		state_transit(S_ACRND_SUSPEND);
	}
	sem_post(&event_sema);
	mngr_send_msg(client_fd, &ack, NULL, 0);
}

static void handle_reboot(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg ack;

	ack.magic = MNGR_MSG_MAGIC;
	ack.msgid = msg->msgid;
	ack.timestamp = msg->timestamp;
	ack.data.err = 0;

	if (msg->data.err) {
		fprintf(stderr, "acrnd refuse to reboot\n");
		state_transit(S_DEFAULT);// wakeup reason will override this anyway
	} else {
		fprintf(stderr, "acrnd agreed to reboot\n");
		state_transit(S_ACRND_REBOOT);
	}
	sem_post(&event_sema);
	mngr_send_msg(client_fd, &ack, NULL, 0);
}

static void handle_wakeup_reason(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg ack;

	ack.magic = MNGR_MSG_MAGIC;
	ack.msgid = msg->msgid;
	ack.timestamp = msg->timestamp;

	ack.data.reason = up_wakeup_reason;
	mngr_send_msg(client_fd, &ack, NULL, 0);
}

static int cbc_timer_format(int *_delta, int *gran)
{
	int delta = *_delta;

	if (delta < 1) {
		fprintf(stderr, "timer as %d second(s), cannot support.\n", delta);
		return -1;
	}
	*gran = 0;
	if (delta > 0xFFFF) {
		delta /= 60;
		*gran = 1;
		goto minute;
	}
	goto done;
minute:
	if (delta > 0xFFFF) {
		delta /= 60;
		*gran = 2;
		goto hour;
	}
	goto done;
hour:
	if (delta > 0xFFFF) {
		delta /= 24;
		*gran = 3;
		goto day;
	}
	goto done;
day:
	if (delta > 0xFFFF) {
		delta /= 7;
		*gran = 4;
		goto week;
	}
	goto done;
week:
	if (delta > 0xFFFF) {
		fprintf(stderr, "timer as %d weeks, cannot support.\n", delta);
		return -1;
	}
done:
	*_delta = delta;
	return 0;
}

static void handle_rtc(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg ack;
	time_t now;
	int delta;
	int gran;

	ack.magic = MNGR_MSG_MAGIC;
	ack.msgid = msg->msgid;
	ack.timestamp = msg->timestamp;

	now = time(NULL);
	delta = msg->data.rtc_timer.t - now;
	if (cbc_timer_format(&delta, &gran) < 0) {
		ack.data.err = -1;
	} else {
		ack.data.err = 0;

		fprintf(stderr, "%s request rtc timer at %lu\n",
			msg->data.rtc_timer.vmname, msg->data.rtc_timer.t);
		cbc_heartbeat_rtc[1] = delta & 0xFF;
		cbc_heartbeat_rtc[2] = (delta & 0xFF00) >> 8;
		cbc_heartbeat_rtc[3] = gran & 0xF;
		cbc_rtc_set = 1;
	}
	mngr_send_msg(client_fd, &ack, NULL, 0);
}

static int send_acrnd_start(void)
{
	int acrnd_fd;
	int ret;
	struct mngr_msg req = {
		.msgid = ACRND_RESUME,
		.magic = MNGR_MSG_MAGIC,
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

/* for non vm manager (acrnd) case, we handle the stop request by ourselves */
static void handle_stop(struct mngr_msg *msg, int client_fd, void *param)
{
	struct mngr_msg *req = (void *)msg;
	struct mngr_msg ack;
	struct mngr_msg req_p = {
		.magic = MNGR_MSG_MAGIC,
	};

	memcpy(&ack, req, sizeof(*req));
	ack.data.err = 0;
	mngr_send_msg(client_fd, &ack, NULL, 0);
	int lcs_fd = mngr_open_un(cbcd_name, MNGR_CLIENT);
	if (lcs_fd < 0) {
		fprintf(stderr, "cannot open sos-lcs.socket\n");
		return;
	}
	if (force_s5)
		req_p.msgid = SHUTDOWN;
	else
		req_p.msgid = SUSPEND;
	mngr_send_msg(lcs_fd, &req_p, NULL, 0);
	mngr_close(lcs_fd);
}

/* return 1 if config file specify "acrnd" exists */
static int check_acrnd(void)
{
	FILE * file = fopen(cbc_match_file, "rb");
	char line[256];
	char device[256];
	char tty[256];
	char acrn[256];
	int is_acrn = 0;

	if (!file)
		goto no_match_file;
	while (1) {
		if (!fgets(line, 255, file))
			goto no_match;
		if (sscanf(line, "%s | %s | %s", device, tty, acrn) != 3)
			goto no_match;
		if (!access(device, F_OK))
			break;
	}
	fclose(file);
	is_acrn = !strncmp(acrn, "acrn", 4);
	return is_acrn;
no_match:
	fclose(file);
no_match_file:
	return is_acrn;
}

static void sigterm_suppress(int sig)
{
	if (!system("systemctl list-jobs reboot.target | grep reboot")) {//reboot
		cbc_send_data(cbc_lifecycle_fd, cbc_heartbeat_reboot,
			sizeof(cbc_heartbeat_reboot));
		exit(0);
	}
	if (!system("systemctl list-jobs poweroff.target | grep poweroff")) {//shutdown
		cbc_send_data(cbc_lifecycle_fd, cbc_heartbeat_shutdown,
			sizeof(cbc_heartbeat_shutdown));
		exit(0);
	}
	cbc_send_data(cbc_lifecycle_fd, cbc_suppress_heartbeat_30min,
			sizeof(cbc_suppress_heartbeat_30min));
	exit(0);
}

int main(void)
{
	cbc_thread_t wakeup_reason_thread_ptr;
	int is_acrn = check_acrnd();
	int v_fd;

	sem_init(&event_sema, 0, 0);
	cbc_lifecycle_fd = open_cbc_device(cbc_lifecycle_dev);
	if (cbc_lifecycle_fd < 0)
		goto err_cbc;
	/* the handle_* function may reply on a close fd, since the client
	 * can close the client_fd and ignore the ack */
	signal(SIGPIPE, SIG_IGN);
	/* if the service is to be killed by SIGTERM, we want cbc send
	 * heartbeat suppress */
	signal(SIGTERM, sigterm_suppress);
	cbcd_fd = mngr_open_un(cbcd_name, MNGR_SERVER);
	if (cbcd_fd < 0) {
		fprintf(stderr, "cannot open %s socket\n", cbcd_name);
		goto err_un;
	}
	if (!is_acrn) {
		v_fd = mngr_open_un(acrnd_name, MNGR_SERVER);
		if (v_fd < 0) {
			fprintf(stderr, "cannot open %s socket\n", acrnd_name);
			goto err_un;
		}
		mngr_add_handler(v_fd, ACRND_STOP, handle_stop, NULL);
	}
	cbc_thread_create(&wakeup_reason_thread_ptr, cbc_wakeup_reason_thread,
			NULL);  // thread to handle wakeup_reason Rx data

	mngr_add_handler(cbcd_fd, WAKEUP_REASON, handle_wakeup_reason, NULL);
	mngr_add_handler(cbcd_fd, RTC_TIMER, handle_rtc, NULL);
	mngr_add_handler(cbcd_fd, SHUTDOWN, handle_shutdown, NULL);
	mngr_add_handler(cbcd_fd, SUSPEND, handle_suspend, NULL);
	mngr_add_handler(cbcd_fd, REBOOT, handle_reboot, NULL);
	cbc_heartbeat_loop();
	// shouldn't be here
	mngr_close(cbcd_fd);
	if (!is_acrn)
		mngr_close(v_fd);
err_un:
	close_cbc_device(cbc_lifecycle_fd);
err_cbc:
	return 0;
}
