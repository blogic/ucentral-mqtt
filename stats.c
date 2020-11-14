/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *   Copyright (C) 2020 John Crispin <john@phrozen.org> 
 */

#include "mqtt.h"

static struct runqueue_process stats;
static struct runqueue runqueue;
static struct blob_buf s;

#define USYNC_STATE	 "/tmp/usync.stats"

static void
runqueue_proc_cb(struct uloop_process *p, int ret)
{
	struct runqueue_process *t = container_of(p, struct runqueue_process, proc);
	void *c;
	char *m;

	runqueue_task_complete(&t->task);

	if(ret) {
		ULOG_ERR("stats task returned %d\n", ret);
		return;
	}

	blob_buf_init(&s, 0);
	blobmsg_add_string(&s, "serial", client.serial);
	c = blobmsg_open_table(&s, "stats");
	if (!blobmsg_add_json_from_file(&s, USYNC_STATE)) {
		ULOG_ERR("failed to load stats\n");
		return;
	}
	blobmsg_close_table(&s, c);

	m = blobmsg_format_json(s.head, true);
	if (m)
		mosquitto_publish(mosq, NULL, client.topic_stats, strlen(m) + 1, m, 0, false);
}


static void
stats_run_cb(struct runqueue *q, struct runqueue_task *task)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
		return;

	if (pid) {
		runqueue_process_add(q, &stats, pid);
		stats.proc.cb = runqueue_proc_cb;
		return;
	}

	execlp("/usr/sbin/usync_stats.sh", "/usr/sbin/usync_stats.sh", NULL);
	exit(1);
}

static const struct runqueue_task_type stats_type = {
	.run = stats_run_cb,
	.cancel = runqueue_process_cancel_cb,
	.kill = runqueue_process_kill_cb,
};

void
stats_send(void)
{
	runqueue_task_add(&runqueue, &stats.task, false);
}

void
stats_init(void)
{
	stats.task.type = &stats_type;
	stats.task.run_timeout = 10 * 1000;
	runqueue_init(&runqueue);
	runqueue.max_running_tasks = 1;
}
