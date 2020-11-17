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

struct client client = {
	.reconnect_timeout = 60,
	.connect_timeout = 60 * 1000,
	.stats_timeout = 60 * 1000,
	.serial = "001122334455",
	.broker = "localhost",
	.port = 8883,
	.venue = "uSync",
	.user = "test",
	.pass = "test",
	.cert = "/etc/usync/mqtt.crt",
	.self_signed = 1,
};

static struct uloop_timeout mqtt_reconnect;
static struct uloop_timeout mqtt_periodic;
static struct uloop_timeout mqtt_connect;
static struct uloop_timeout mqtt_stats;
struct mosquitto *mosq;

static void
mosq_connect_cb(struct mosquitto *mosq, void *obj, int result)
{
	ULOG_INFO("connected\n");

	client.reconnect_timeout = 30;
	client.conn_time = time(NULL);
	client.connected = 1;
	uloop_timeout_set(&mqtt_stats, client.stats_timeout);
	mosquitto_subscribe(mosq, NULL, client.topic_venue, 0);
	mosquitto_subscribe(mosq, NULL, client.topic_cmd, 0);
}

static void
mosq_disconnect_cb(struct mosquitto *mosq, void *obj, int result)
{
	ULOG_INFO("disconnected\n");
	client.conn_time = time(NULL);
	client.connected = 0;
	uloop_timeout_cancel(&mqtt_stats);
	uloop_timeout_set(&mqtt_reconnect, client.connect_timeout);
}

static void
mosq_msg_cb(struct mosquitto *mosq, void *obj,
	    const struct mosquitto_message *message)
{
	bool match = 0;

	printf("%s - %.*s\n", message->topic, message->payloadlen, (char *)message->payload);

	mosquitto_topic_matches_sub(client.topic_venue, message->topic, &match);
	if (match)
		mqtt_notify((char *)message->payload, message->payloadlen);
	mosquitto_topic_matches_sub(client.topic_cmd, message->topic, &match);
	if (match)
		printf("cmd\n");
}

static void
mosq_cb_log(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	int lvl = LOG_INFO;

	switch (level) {
	case MOSQ_LOG_DEBUG:
		lvl = LOG_DEBUG;
		break;
	case MOSQ_LOG_NOTICE:
		lvl = LOG_NOTICE;
		break;
	case MOSQ_LOG_WARNING:
		lvl = LOG_WARNING;
		break;
	case MOSQ_LOG_ERR:
		lvl = LOG_ERR;
		break;
	}
	ulog(lvl, "%s\n", str);
}

static void
mqtt_connect_cb(struct uloop_timeout *t)
{
	if (!mosquitto_connect(mosq, client.broker, client.port, 60))
		return;
	ULOG_INFO("failed to connect\n");
	uloop_timeout_set(t, client.connect_timeout);
}

static void
mqtt_reconnect_cb(struct uloop_timeout *t)
{
	if (!mosquitto_reconnect(mosq))
		return;
	client.reconnect_timeout *= 2;
	client.reconnect_timeout %= 15 * 60;
	uloop_timeout_set(t, client.connect_timeout);
}

static void
mqtt_periodic_cb(struct uloop_timeout *t)
{
	mosquitto_loop(mosq, 100, 1);
	uloop_timeout_set(t, 100);
}

static void
mqtt_stats_cb(struct uloop_timeout *t)
{
	stats_send();
	uloop_timeout_set(t, client.stats_timeout);
}

int
mqtt_publish(struct blob_attr *msg)
{
	char *str;

	if (!client.connected)
		return -1;

	str = blobmsg_format_json(msg, true);
	if (!str)
		return -1;

	mosquitto_publish(mosq, NULL, client.topic_venue, strlen(str) + 1, str, 0, false);

	return 0;
}

static void
mqtt_init(void)
{
	mosquitto_lib_init();
	mosq = mosquitto_new(client.serial, true, 0);
	mosquitto_connect_callback_set(mosq, mosq_connect_cb);
	mosquitto_disconnect_callback_set(mosq, mosq_disconnect_cb);
	mosquitto_message_callback_set(mosq, mosq_msg_cb);
	mosquitto_log_callback_set(mosq, mosq_cb_log);
	if (client.user && client.pass)
		mosquitto_username_pw_set(mosq, client.user, client.pass);
	if (client.cert) {
		mosquitto_tls_set(mosq, client.cert, NULL, NULL, NULL, NULL);
		mosquitto_tls_insecure_set(mosq, client.self_signed);
	}
	mqtt_stats.cb = mqtt_stats_cb;
	mqtt_periodic.cb = mqtt_periodic_cb;
	mqtt_connect.cb = mqtt_connect_cb;
	mqtt_reconnect.cb = mqtt_reconnect_cb;
	uloop_timeout_set(&mqtt_connect, 1000);
	uloop_timeout_set(&mqtt_periodic, 500);
}

static int
print_usage(const char *daemon)
{
	fprintf(stderr, "Usage: %s [options]\n"
			"\t-S <serial>\n"
			"\t-u <username>\n"
			"\t-p <password>\n"
			"\t-s <server>\n"
			"\t-P <port>\n"
			"\t-d <debug>\n"
			"\t-c <cert>\n"
			"\t-i <allow self signed>\n"
			"\t-v <venue>\n", daemon);
	return -1;
}

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "S:u:p:s:P:v:c:di")) != -1) {
		switch (ch) {
		case 'u':
			client.user = optarg;
			break;
		case 'p':
			client.pass = optarg;
			break;
		case 's':
			client.broker = optarg;
			break;
		case 'c':
			client.cert = optarg;
			break;
		case 'P':
			client.port = atoi(optarg);
			break;
		case 'd':
			client.debug = 1;
			break;
		case 'i':
			client.self_signed = 1;
			break;
		case 'v':
			client.venue = optarg;
			break;
		case 'S':
			client.serial = optarg;
			break;
		case 'h':
		default:
			return print_usage(*argv);
		}
	}
	snprintf(client.topic_stats, sizeof(client.topic_stats), "%s/stats", client.venue);
	snprintf(client.topic_venue, sizeof(client.topic_venue), "%s/venue", client.venue);
	snprintf(client.topic_cmd, sizeof(client.topic_cmd), "%s/cmd", client.serial);

	client.conn_time = time(NULL);

	ulog_open(ULOG_STDIO | ULOG_SYSLOG, LOG_DAEMON, "usync-mqtt");
	if (!client.debug)
		ulog_threshold(LOG_INFO);

	uloop_init();
	stats_init();
	ubus_init();
	mqtt_init();
	uloop_run();

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}
