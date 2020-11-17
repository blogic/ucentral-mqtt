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

#include <uuid/uuid.h>

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mosquitto.h>

static int loop;
static struct mosquitto *mosq;
static int rc = 0;
static char *serial;
static char *broker;
static int port = 1883;
static char *venue;
static char *user;
static char *pass;
static char *cert;
static char topic_stats[64];
static char topic_cmd[64];
static int self_signed;
static int debug;
static int log;
static uuid_t buuid;
static char uuid[37];
static char *cmd;

static void
handle_signal(int s)
{
	loop = 0;
}

static void
mosq_connect_cb(struct mosquitto *mosq, void *obj, int result)
{
	fprintf(stderr, "connected\n");
	if (cmd && serial) {
		fprintf(stderr, "issuing command\n");
		mosquitto_publish(mosq, NULL, topic_cmd, strlen(cmd) + 1, cmd, 0, false);
	}
}

static void
mosq_disconnect_cb(struct mosquitto *mosq, void *obj, int result)
{
	fprintf(stderr, "disconnected\n");
}

static void
mosq_publish_cb(struct mosquitto *mosq, void *obj, int result)
{
	fprintf(stderr, "command was published\n");
	cmd = NULL;
	if (!log)
		loop = 0;
}

static void
mosq_cb_log(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	if (level ==  MOSQ_LOG_DEBUG && !debug)
		return;
	fprintf(stderr, "%s\n", str);
}

static void
mosq_msg_cb(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	printf("%s - %.*s\n", message->topic, message->payloadlen, (char *)message->payload);
}

static int
print_usage(char *p)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"\t-S <serial>\n"
		"\t-u <username>\n"
		"\t-p <password>\n"
		"\t-s <server>\n"
		"\t-P <port>\n"
		"\t-c <cert>\n"
		"\t-v <venue>\n"
		"\t-C <command>\n"
		"\t-d - enable debug\n"
		"\t-i - allow self signed\n"
		"\t-l - subscribe to logs\n",
		p);
	return -1;
}

int
main(int argc, char *argv[])
{
	char client[64];
	int ch;

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	while ((ch = getopt(argc, argv, "S:u:p:s:P:v:c:C:dil")) != -1) {
		switch (ch) {
		case 'u':
			user = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 's':
			broker = optarg;
			break;
		case 'c':
			cert = optarg;
			break;
		case 'C':
			cmd = optarg;
			break;
		case 'P':
			port = atoi(optarg);
			break;
		case 'v':
			venue = optarg;
			break;
		case 'S':
			serial = optarg;
			break;
		case 'd':
			debug = 1;
			break;
		case 'i':
			self_signed = 1;
			break;
		case 'l':
			log = 1;
			break;
		case 'h':
		default:
			return print_usage(*argv);
		}
	}

	if (!venue || !broker)
		return print_usage(*argv);

	uuid_generate(buuid);
	uuid_unparse(buuid, uuid);
	snprintf(client, sizeof(client), "cli-%s", uuid);
	fprintf(stderr, "using client id %s\n", client);
	mosquitto_lib_init();
	mosq = mosquitto_new(client, true, 0);
	mosquitto_connect_callback_set(mosq, mosq_connect_cb);
	mosquitto_disconnect_callback_set(mosq, mosq_disconnect_cb);
	mosquitto_message_callback_set(mosq, mosq_msg_cb);
	mosquitto_log_callback_set(mosq, mosq_cb_log);
	mosquitto_publish_callback_set(mosq, mosq_publish_cb);
	if (user && pass)
		mosquitto_username_pw_set(mosq, user, pass);
	if (cert) {
		mosquitto_tls_set(mosq, cert, NULL, NULL, NULL, NULL);
		mosquitto_tls_insecure_set(mosq, self_signed);
	}
	rc = mosquitto_connect(mosq, broker, port, 60);

	if (log) {
		snprintf(topic_stats, sizeof(topic_stats), "%s/stats", venue);
		mosquitto_subscribe(mosq, NULL, topic_stats, 0);
		loop = 1;
	}

	if (cmd && serial) {
		snprintf(topic_cmd, sizeof(topic_cmd), "%s/cmd", serial);
		loop = 1;
	}

	if (!loop)
		return print_usage(*argv);

	while (loop) {
		rc = mosquitto_loop(mosq, -1, 1);
		if (loop && rc) {
			fprintf(stderr, "connection error!\n");
			sleep(10);
			mosquitto_reconnect(mosq);
		}
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}
