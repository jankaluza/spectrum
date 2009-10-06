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

typedef struct _PurpleIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
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

	channel = g_io_channel_unix_new(fd);
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

PurpleEventLoopUiOps * getEventLoopUiOps(void){
	return &eventLoopOps;
}
