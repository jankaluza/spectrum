/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "geventloop.h"
#include "transport_config.h"
#include "transport.h"
#ifdef _WIN32
#include "win32/win32dep.h"
#endif
#ifdef WITH_LIBEVENT
#include "ev.h"
#endif

static struct ev_loop *loop;

typedef struct _PurpleIOClosure {
	// This has to be first member of this struct because of casting
#ifdef WITH_LIBEVENT
	struct ev_io io;
#endif
	PurpleInputFunction function;
	guint result;
	gpointer data;
#ifdef WITH_LIBEVENT
	GSourceFunc function2;
	struct ev_timer timer;
#endif
} PurpleIOClosure;

static gboolean io_invoke(GIOChannel *source,
										GIOCondition condition,
										gpointer data)
{
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	PurpleInputCondition purple_cond = (PurpleInputCondition)0;

	int tmp = 0;
	if (condition & READ_COND)
	{
		tmp |= PURPLE_INPUT_READ;
		purple_cond = (PurpleInputCondition)tmp;
	}
	if (condition & WRITE_COND)
	{
		tmp |= PURPLE_INPUT_WRITE;
		purple_cond = (PurpleInputCondition)tmp;
	}

	closure->function(closure->data, g_io_channel_unix_get_fd(source), purple_cond);

	return TRUE;
}

static void io_destroy(gpointer data)
{
	g_free(data);
}

static guint input_add(gint fd,
								PurpleInputCondition condition,
								PurpleInputFunction function,
								gpointer data)
{
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = (GIOCondition)0;
	closure->function = function;
	closure->data = data;

	int tmp = 0;
	if (condition & PURPLE_INPUT_READ)
	{
		tmp |= READ_COND;
		cond = (GIOCondition)tmp;
	}
	if (condition & PURPLE_INPUT_WRITE)
	{
		tmp |= WRITE_COND;
		cond = (GIOCondition)tmp;
	}

#ifdef WIN32
	channel = wpurple_g_io_channel_win32_new_socket(fd);
#else
	channel = g_io_channel_unix_new(fd);
#endif
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
	io_invoke, closure, io_destroy);

	g_io_channel_unref(channel);
	return closure->result;
}

static PurpleEventLoopUiOps eventLoopOps =
{
	g_timeout_add,
	g_source_remove,
	input_add,
	g_source_remove,
	NULL,
#if GLIB_CHECK_VERSION(2,14,0)
	g_timeout_add_seconds,
#else
	NULL,
#endif

	NULL,
	NULL,
	NULL
};

#ifdef WITH_LIBEVENT

static GHashTable *events = NULL;
static unsigned long id = 0;

static void event_io_destroy(gpointer data)
{
	PurpleIOClosure *closure = (PurpleIOClosure* )data;
	ev_io_stop(loop, &closure->io);
	ev_timer_stop(loop, &closure->timer);
	g_free(data);
}

static void event_timer_invoke(struct ev_loop *l, struct ev_timer *w, int event) {
	PurpleIOClosure *closure = (PurpleIOClosure *) (((char *)w) - offsetof (PurpleIOClosure, timer));

	if (closure->function2(closure->data))
		ev_timer_again(l, w);
}

static void event_io_invoke(struct ev_loop *l, struct ev_io *w, int event) {
	PurpleIOClosure *closure = (PurpleIOClosure* )w;
	PurpleInputCondition purple_cond = (PurpleInputCondition)0;
	int tmp = 0;
	if (event & EV_READ)
	{
		tmp |= PURPLE_INPUT_READ;
		purple_cond = (PurpleInputCondition)tmp;
	}
	if (event & EV_WRITE)
	{
		tmp |= PURPLE_INPUT_WRITE;
		purple_cond = (PurpleInputCondition)tmp;
	}

	closure->function(closure->data, w->fd, purple_cond);
}

static gboolean event_input_remove(guint handle)
{
	PurpleIOClosure *closure = (PurpleIOClosure *) g_hash_table_lookup(events, &handle);
	if (closure) {
		event_io_destroy(closure);
	}
	return TRUE;
}

static guint event_input_add(gint fd,
								PurpleInputCondition condition,
								PurpleInputFunction function,
								gpointer data)
{
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	closure->function = function;
	closure->data = data;

	int tmp = 0;
	if (condition & PURPLE_INPUT_READ)
	{
		tmp |= EV_READ;
	}
	if (condition & PURPLE_INPUT_WRITE)
	{
		tmp |= EV_WRITE;
	}

	ev_io_init(&closure->io, event_io_invoke, fd, tmp);
	ev_io_start(loop, &closure->io);

	int *f = (int *) g_malloc(sizeof(int));
	*f = id++;
	g_hash_table_replace(events, f, closure);
	
	return *f;
}

static guint event_timeout_add (guint interval, GSourceFunc function, gpointer data) {
	struct timeval timeout;
	PurpleIOClosure *closure = g_new0(PurpleIOClosure, 1);
	closure->function2 = function;
	closure->data = data;

	ev_timer_init (&closure->timer, event_timer_invoke, (double)(interval) / 1000, 0.);
	ev_timer_start (loop, &closure->timer);
	
	guint *f = (guint *) g_malloc(sizeof(guint));
	*f = id++;
	g_hash_table_replace(events, f, closure);
	return *f;
}

static PurpleEventLoopUiOps libEventLoopOps =
{
	event_timeout_add,
	event_input_remove,
	event_input_add,
	event_input_remove,
	NULL,
// #if GLIB_CHECK_VERSION(2,14,0)
// 	g_timeout_add_seconds,
// #else
	NULL,
// #endif

	NULL,
	NULL,
	NULL
};

#endif /* WITH_LIBEVENT*/

PurpleEventLoopUiOps * getEventLoopUiOps(){
	if (CONFIG().eventloop == "glib")
		return &eventLoopOps;
#ifdef WITH_LIBEVENT
	events = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL);
	return &libEventLoopOps;
#endif
}

void setLoop(struct ev_loop *l) {
	loop = l;
}
