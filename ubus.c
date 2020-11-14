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

static struct ubus_object ubus_object;
static struct ubus_auto_conn conn;
static int has_subscribers;
static struct blob_buf b;

static void
ubus_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj)
{
	has_subscribers = ubus_object.has_subscribers;
}

static int
ubus_publish_cb(struct ubus_context *ctx,
		struct ubus_object *obj,
		struct ubus_request_data *req,
		const char *method, struct blob_attr *msg)
{
	if (mqtt_publish(msg))
		return UBUS_STATUS_TIMEOUT;
	return UBUS_STATUS_OK;
}

static int
ubus_state_cb(struct ubus_context *ctx,
	      struct ubus_object *obj,
	      struct ubus_request_data *req,
	      const char *method, struct blob_attr *msg)
{
	time_t delta = time(NULL) - client.conn_time;

	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, client.connected ? "connected" : "disconnected", delta);
	blobmsg_add_string(&b, "topic_stats", client.topic_stats);
	blobmsg_add_string(&b, "topic_venue", client.topic_venue);
	ubus_send_reply(ctx, req, b.head);
	return UBUS_STATUS_OK;
}

static const struct ubus_method usync_mqtt_methods[] = {
	UBUS_METHOD_NOARG("state", ubus_state_cb),
	UBUS_METHOD_NOARG("publish", ubus_publish_cb),
};

static struct ubus_object_type ubus_object_type =
	UBUS_OBJECT_TYPE("mqtt", usync_mqtt_methods);

static struct ubus_object ubus_object = {
	.name = "mqtt",
	.type = &ubus_object_type,
	.methods = usync_mqtt_methods,
	.n_methods = ARRAY_SIZE(usync_mqtt_methods),
	.subscribe_cb = ubus_subscribe_cb,
};

void
mqtt_notify(char *msg, int len)
{
	if (msg[len - 1])
		return;

	blob_buf_init(&b, 0);
	if (blobmsg_add_json_from_string(&b, msg))
		blobmsg_add_string(&b, "msg", msg);
	ubus_notify(&conn.ctx, &ubus_object, "msg", b.head, -1);
}

static void
ubus_connect_handler(struct ubus_context *ctx)
{
	ubus_add_object(ctx, &ubus_object);
}

void
ubus_init(void)
{
	conn.cb = ubus_connect_handler;
	ubus_auto_connect(&conn);
}
