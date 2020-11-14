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

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <mosquitto.h>

#include <libubox/blobmsg_json.h>
#include <libubox/runqueue.h>
#include <libubox/uloop.h>
#include <libubox/ulog.h>

#include <libubus.h>

struct client {
	int connect_timeout;
	int stats_timeout;
	char *serial;
	char *broker;
	int port;
	char *venue;
	char *user;
	char *pass;
	char *cert;
	int self_signed;
	int debug;
	char topic_venue[64];
	char topic_stats[64];
	int connected;
	time_t conn_time;
};

extern struct mosquitto *mosq;
extern struct client client;

void ubus_init(void);
int mqtt_publish(struct blob_attr *msg);
void mqtt_notify(char *msg, int len);
void stats_init(void);
void stats_send(void);
