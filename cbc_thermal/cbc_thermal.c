/*
 * Copyright (C) 2018 Intel Corporation
 *
 * Author: Bin Yang<bin.yang@intel.com>
 *
 * SPDX-License-identifier: BSD-3-Clause
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

//#define DEBUG
#define pr_log(fmt, ...) do { \
		struct timeval tv; \
		gettimeofday(&tv, NULL); \
                uint64_t _us = (tv.tv_sec * 1000000 + tv.tv_usec); \
                printf("[CBC Thermal] %06d.%06d " fmt, \
			(int)((_us / 1000000) % 1000000), (int)(_us % 1000000), ## __VA_ARGS__); \
		fflush(stdout); \
        } while(0)
#define ASSERT(cond, fmt, ...) do {\
		if (!(cond)) { \
			pr_log("ASSERT " fmt "\n", ## __VA_ARGS__); \
			exit(1); \
		} \
        } while(0)
#ifdef DEBUG
#define pr_dbg pr_log
#define pr_dump(_buf, _len, args...) do { \
		int _i, _j; \
		pr_log(args); \
		for (_i = 0; _i < _len / 32 + 1; _i ++) {\
			printf("\t"); \
			for (_j = 0; _j + (_i * 32) < _len; _j ++) \
				printf("%02x ", _buf[_j + (_i * 32)]); \
			printf("\n"); \
		} \
		printf("\n"); \
		fflush(stdout); \
        } while(0)
#else
#define pr_dbg(fmt, ...) do{}while(0)
#define pr_dump(...) do{}while(0)
#endif

#define TH_IO_DIR "/run/cbc_thermal"
#define TH_IOBUF_MAX 64

#define IO_FOREACH(_i, _io) for (_i = 0, _io = &io_inits[0]; _i < IO_INITS_NUM; _i++, _io++)

struct cbc_th_io {
	char *name;
	void *data;
	int (*read)(char *buf, int len, void *data);
	int (*write)(char *buf, int len, void *data);
};

static int cbc_th_io_ready;
static int cbc_diagnosis_fd, cbc_signals_fd;
static int cbc_fan0_val, cbc_amplifier_temp_val, cbc_env_temp_val, cbc_ambient_temp_val;

static inline void write_exact(int fd, void *buf, int len)
{
	int ret;

	while (len > 0) {
		ret = write(fd, buf, len);
		ASSERT(ret >= 0, "write error, ret=%d\n", ret);
		len -= ret;
	}
}

static void *cbc_read_thread(void *arg)
{
	int len;
	unsigned char buf[4096];
	fd_set rfd;
	int max_fd = cbc_signals_fd > cbc_diagnosis_fd ? cbc_signals_fd : cbc_diagnosis_fd;

	write_exact(cbc_signals_fd, "\xff", 1);
	write_exact(cbc_diagnosis_fd, "\x08\x64", 2);

	cbc_th_io_ready = 1;
	while (1) {
		FD_ZERO(&rfd);
		FD_SET(cbc_signals_fd, &rfd);
		FD_SET(cbc_diagnosis_fd, &rfd);
		select(max_fd + 1, &rfd, NULL, NULL, NULL);
		if (FD_ISSET(cbc_signals_fd, &rfd)) {
			int i, num;
			unsigned char *sig;
			unsigned short sig_id;
			unsigned int sig_val;

			len = read(cbc_signals_fd, buf, sizeof(buf));
			pr_dump(buf, len, "cbc_signals: ");
			if (len < 4 || buf[0] != 0x2)
				continue;
			num = buf[1];
			sig = &buf[2];
			pr_dbg("sig num=%d\n", num);
			for (sig = &buf[2], i = 0; i < num; sig += 6, i ++) {
				sig_id = sig[0] + (sig[1] << 8);
				sig_val = sig[2] + (sig[3] << 8);
				pr_dbg("sig: id=%d, val=%x\n", sig_id, sig_val);
				if (sig_id == 502) {
					cbc_amplifier_temp_val = (sig_val / 100) - 100;
					pr_dbg("cbc_amplifier_temp_val=%x\n", cbc_amplifier_temp_val);
				}
				if (sig_id == 503) {
					cbc_env_temp_val = (sig_val / 100) - 100;
					pr_dbg("cbc_env_temp_val=%x\n", cbc_env_temp_val);
				}
				if (sig_id == 870) {
					cbc_ambient_temp_val = (sig_val / 100) - 100;
					pr_dbg("cbc_ambient_temp_val=%x\n", cbc_ambient_temp_val);
				}
			}
		}
		if (FD_ISSET(cbc_diagnosis_fd, &rfd)) {
			len = read(cbc_diagnosis_fd, buf, sizeof(buf));
			pr_dump(buf, len, "cbc_diagnosis: ");
			if (len == 4 && buf[0] == 0x9) {
				pr_dbg("cbc fan0 duty: %x\n", cbc_fan0_val);
				cbc_fan0_val = buf[1];
			}
		}
	}
	return NULL;
}

static int cbc_amplifier_temp_read(char *buf, int len, void *data)
{
	int ret = snprintf(buf, len, "%d", cbc_amplifier_temp_val);
	pr_dbg("%s: %s\n", __func__, buf);
	return ret;
}

static int cbc_env_temp_read(char *buf, int len, void *data)
{
	int ret = snprintf(buf, len, "%d", cbc_env_temp_val);
	pr_dbg("%s: %s\n", __func__, buf);
	return ret;
}

static int cbc_ambient_temp_read(char *buf, int len, void *data)
{
	int ret = snprintf(buf, len, "%d", cbc_ambient_temp_val);
	pr_dbg("%s: %s\n", __func__, buf);
	return ret;
}

static int cbc_fan0_read(char *buf, int len, void *data)
{
	int ret = snprintf(buf, len, "%d", cbc_fan0_val);
	pr_dbg("%s: %s\n", __func__, buf);
	return ret;
}

static int cbc_fan0_write(char *buf, int len, void *data)
{
	unsigned char cmd[] = {0x08, 0};
	cmd[1] = (unsigned char)atoi(buf);

	pr_log("%s: duty=%d\n", __func__, (int)cmd[1]);
	write_exact(cbc_diagnosis_fd, cmd, sizeof(cmd));
	return len;
}

#define IO_INITS_NUM (sizeof(io_inits)/sizeof(io_inits[0]))
static struct cbc_th_io io_inits[] = {
/* sensors */
	{
		/* IasTemperatureSensorAmplifier */
		.name = "cbc_amplifier_temp",
		.read = cbc_amplifier_temp_read,
	},
	{
		/* IasTemperatureSensorEnvironment */
		.name = "cbc_env_temp",
		.read = cbc_env_temp_read,
	},
	{
		/* IasAmbientTemperature */
		.name = "cbc_ambient_temp",
		.read = cbc_ambient_temp_read,
	},
/* cooling devices */
	{
		.name = "cbc_fan0",
		.read = cbc_fan0_read,
		.write = cbc_fan0_write,
	},
};

static int io_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		stbuf->st_size = 4096;
	}
	return res;
}

static int io_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	int i;
	struct cbc_th_io *io;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	IO_FOREACH(i, io) {
		filler(buf, io->name, NULL, 0);
	}
	return 0;
}

static int io_truncate(const char *path, off_t size)
{
        return 0;
}

static int io_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int i;
	struct cbc_th_io *io;

	IO_FOREACH(i, io) {
		if(strcmp(path + 1, io->name) == 0) {
			if (io->write)
				io->write((char *)buf, size, io->data);
			else
				pr_log("%s: invalid write: %s\n", io->name, buf);
			break;
		}
	}
	return size;
}

static int io_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int len = 0;
	int i;
	struct cbc_th_io *io;

	buf[0] = 0;
	IO_FOREACH(i, io) {
		if(strcmp(path + 1, io->name) == 0) {
			if (offset == 0 && io->read)
				len = io->read(buf, size, io->data);
			break;
		}
	}
	return len;
}

static struct fuse_operations cbc_thermal_fsops = {
	.getattr	= io_getattr,
	.readdir	= io_readdir,
	.read		= io_read,
	.write		= io_write,
        .truncate	= io_truncate,
};

int main(void)
{
	pthread_t pthread;
	pthread_attr_t attr;
        char *fake_argv[10];

	pr_log("wait for cbc device ...\n");
	while (1) {
		if (access("/dev/cbc-diagnosis", F_OK) == 0 && access("/dev/cbc-signals", F_OK) == 0)
			break;
		sleep(1);
	}

	pr_log("open cbc-diagnosis ...\n");
	cbc_diagnosis_fd = open("/dev/cbc-diagnosis", O_RDWR | O_NOCTTY);
	ASSERT(cbc_diagnosis_fd > 0, "open /dev/cbc-diagnosis error\n");

	pr_log("open cbc-signals ...\n");
	cbc_signals_fd = open("/dev/cbc-signals", O_RDWR | O_NOCTTY);
	ASSERT(cbc_signals_fd > 0, "open /dev/cbc-signals error\n");

	signal(SIGPIPE, SIG_IGN);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&pthread, (const pthread_attr_t *)&attr, cbc_read_thread, NULL) != 0)
		ASSERT(0, "create cbc thermal thread error\n");

	pr_log("wait for cbc thermal io ready ...\n");
	while (!cbc_th_io_ready)
		sleep(1);

	pr_log("mount cbc thermal io ...\n");
	if (system("mkdir -p " TH_IO_DIR) != 0)
		ASSERT(0, "mkdir -p "TH_IO_DIR " error!!!\n");
        fake_argv[0] = "cbc_thermal";
        fake_argv[1] = "-f";
        fake_argv[2] = "-o";
        fake_argv[3] = "nonempty";
        fake_argv[4] = "-o";
        fake_argv[5] = "allow_other";
        fake_argv[6] = "-o";
        fake_argv[7] = "default_permissions";
        fake_argv[8] = TH_IO_DIR;
        return fuse_main(9, fake_argv, &cbc_thermal_fsops, NULL);
}
