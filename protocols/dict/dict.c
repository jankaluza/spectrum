/**
 * TODO: legal stuff
 *
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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

#include <glib.h>
#include "dict.h"

#define WORD_DEFINITION "\n151"
#define WORD_MATCH "\n152"
#define WORD_NOT_FOUND "\n552"

static PurplePlugin *_dict_protocol = NULL;

static void dummy_authorization_cb(void * data) { }

static char *dict_ask_server(const char *conv_name, const char *message, const char *type) {
	FILE *fp;
	int status;
	char line[4096] = "";
	char *result;
	int len = 4096;
	char *query = g_strdup_printf("curl 'dict://dict.org/%s:%s:%s'", type, message, conv_name);

	result = g_malloc(sizeof(char) * 4096);
	strcpy(result, "");
	
	fp = popen(query, "r");
	g_free(query);
	if (fp == NULL)
		return 0;

	while (fgets(line, 4096, fp) != NULL) {
		strncat(result, line, len);
		len -= strlen(line);
	}
	status = pclose(fp);
	return result;
}

static char * dict_get_first_match(const char *conv_name, const char *message) {
	char *ptr;
	char *response = g_malloc(sizeof(char) * 4096);
	strcpy(response, "");

	char *result = dict_ask_server(conv_name, message, "m");
	if (!result)
		return 0;
	ptr = result;

	if ((ptr = strstr(ptr, WORD_MATCH)) == NULL)
		return 0;

	ptr += 1;
	if ((ptr = strchr(ptr, '\n')) == NULL)
		return 0;
	ptr++;
	
	if (*ptr == '.')
		return 0;

	ptr += strlen(conv_name) + 2;
	strncat(response, ptr, strchr(ptr, '"') - ptr);

	g_free(result);
	return response;
}

static int dict_send_im(PurpleConnection *gc, const char *conv_name, const char *message, PurpleMessageFlags flags) {
	char *response = g_malloc(sizeof(char) * 4096);;
	char *ptr;

	if (strlen(message) > 255)
		return -E2BIG;

	char *result = dict_ask_server(conv_name, message, "d");
	strcpy(response, "");
	if (!result)
		return 0;
	ptr = result;
	
	if (strstr(ptr, WORD_NOT_FOUND) != NULL) {
		char *match = dict_get_first_match(conv_name, message);
		if (match && *match!='\0') {
			g_free(result);
			char *result = dict_ask_server(conv_name, match, "d");
			ptr = result;
			strcat(response, "Did you mean '");
			strcat(response, match);
			strcat(response, "'?\n");
		}
		if (match)
			g_free(match);
	}
	
	int i;
	while ((ptr = strstr(ptr, WORD_DEFINITION)) != NULL) {
		ptr += 1;
		for (i = 0; i < 3; i++) {
			if ((ptr = strchr(ptr, '\n')) == NULL) break;
			ptr++;
		}
		if (!ptr) break;

		if (strlen(response) + strchr(ptr, '\n') - ptr + 1 > 4095)
			break;
		strncat(response, ptr, strchr(ptr, '\n') - ptr);
		strcat(response, "\n");
	}
	
	serv_got_im(gc, conv_name, response, PURPLE_MESSAGE_RECV, time(NULL));
	g_free(result);
	g_free(response);

	return 0;
}


static void dict_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group) {
	purple_account_request_authorization(purple_buddy_get_account(buddy), buddy->name, buddy->name, buddy->name, "Authorize me.", 1, dummy_authorization_cb, dummy_authorization_cb, NULL);
	purple_prpl_got_user_status(purple_buddy_get_account(buddy), buddy->name, "online", "message", "", NULL);
}

static GList *dict_status_types(PurpleAccount *account) {
	GList *types = NULL;
	PurpleStatusType *type;
	PurpleStatusPrimitive status_primitives[] = {
		PURPLE_STATUS_UNAVAILABLE,
		PURPLE_STATUS_INVISIBLE,
		PURPLE_STATUS_AWAY,
		PURPLE_STATUS_EXTENDED_AWAY
	};
	int status_primitives_count = sizeof(status_primitives) / sizeof(status_primitives[0]);
	int i;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, "online",
			"online", TRUE);
	purple_status_type_add_attr(type, "message", ("Online"),
			purple_value_new(PURPLE_TYPE_STRING));
	types = g_list_prepend(types, type);

	//This is a hack to get notified when another protocol goes into a different status.
	//Eg aim goes "away", we still want to get notified
	for (i = 0; i < status_primitives_count; i++)
	{
		type = purple_status_type_new(status_primitives[i], "online",
				"online", FALSE);
		purple_status_type_add_attr(type, "message", ("Online"),
				purple_value_new(PURPLE_TYPE_STRING));
		types = g_list_prepend(types, type);
	}

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, "offline",
			"offline", TRUE);
	purple_status_type_add_attr(type, "message", ("Offline"),
			purple_value_new(PURPLE_TYPE_STRING));
	types = g_list_prepend(types, type);

	return g_list_reverse(types);
}

static void dict_login(PurpleAccount *account) {
	PurpleConnection *gc = purple_account_get_connection(account);
	gc->proto_data = NULL;

	purple_debug_info(DICT_PROTOCOL_ID, "logging in %s\n", account->username);

	/* purple wants a minimum of 2 steps */
	purple_connection_update_progress(gc, ("Connected"),
			1,   /* which connection step this is */
			2);  /* total number of steps */
	purple_connection_set_state(gc, PURPLE_CONNECTED);

	GSList *buddies = purple_find_buddies(account, NULL);
	while(buddies) {
		PurpleBuddy *buddy = (PurpleBuddy *) buddies->data;
		purple_prpl_got_user_status(purple_buddy_get_account(buddy), buddy->name, "online", "message", "", NULL);
		buddies = g_slist_delete_link(buddies, buddies);
	}
	
}

static void dict_close(PurpleConnection *gc) {

}

static void dict_init(PurplePlugin *plugin) {
	purple_debug_info(DICT_PROTOCOL_ID, "starting up\n");

	_dict_protocol = plugin;
}

static const char *dict_list_icon(PurpleAccount *account, PurpleBuddy *buddy) {
	return "prpldict";
}

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_CHAT_TOPIC,  /* options */
	NULL,	       /* user_splits, initialized in twitter_init() */
	NULL,	       /* protocol_options, initialized in twitter_init() */
	{   /* icon_spec, a PurpleBuddyIconSpec */
		"png,jpg,gif",		   /* format */
		0,			       /* min_width */
		0,			       /* min_height */
		48,			     /* max_width */
		48,			     /* max_height */
		10000,			   /* max_filesize */
		PURPLE_ICON_SCALE_DISPLAY,       /* scale_rules */
	},
	dict_list_icon,		  /* list_icon */ //TODO
	NULL, //twitter_list_emblem,		/* list_emblem */
	NULL,		/* status_text */
	NULL,	       /* tooltip_text */
	dict_status_types,	       /* status_types */
	NULL,	    /* blist_node_menu */
	NULL,		  /* chat_info */
	NULL,	 /* chat_info_defaults */
	dict_login,		      /* login */
	dict_close,		      /* close */
	dict_send_im, //twitter_send_dm,		    /* send_im */
	NULL,		   /* set_info */
	NULL, //twitter_send_typing,		/* send_typing */
	NULL,		   /* get_info */
	NULL,		 /* set_status */
	NULL,		   /* set_idle */
	NULL,//TODO?	      /* change_passwd */
	dict_add_buddy,//TODO		  /* add_buddy */
	NULL,//TODO		/* add_buddies */
	NULL,//TODO	       /* remove_buddy */
	NULL,//TODO	     /* remove_buddies */
	NULL,//TODO?		 /* add_permit */
	NULL,//TODO?		   /* add_deny */
	NULL,//TODO?		 /* rem_permit */
	NULL,//TODO?		   /* rem_deny */
	NULL,//TODO?	    /* set_permit_deny */
	NULL,		  /* join_chat */
	NULL,		/* reject_chat */
	NULL,	      /* get_chat_name */
	NULL,		/* chat_invite */
	NULL,		 /* chat_leave */
	NULL,//twitter_chat_whisper,	       /* chat_whisper */
	NULL,		  /* chat_send */
	NULL,//TODO?				/* keepalive */
	NULL,	      /* register_user */
	NULL,		/* get_cb_info */
	NULL,				/* get_cb_away */
	NULL,//TODO?		/* alias_buddy */
	NULL,		/* group_buddy */
	NULL,	       /* rename_group */
	NULL,				/* buddy_free */
	NULL,	       /* convo_closed */
	NULL,		  /* normalize */
	NULL,	     /* set_buddy_icon */
	NULL,	       /* remove_group */
	NULL,//TODO?				/* get_cb_real_name */
	NULL,	     /* set_chat_topic */
	NULL,				/* find_blist_chat */
	NULL,	  /* roomlist_get_list */
	NULL,	    /* roomlist_cancel */
	NULL,   /* roomlist_expand_category */
	NULL,	   /* can_receive_file */
	NULL,				/* send_file */
	NULL,				/* new_xfer */
	NULL,	    /* offline_message */
	NULL,				/* whiteboard_prpl_ops */
	NULL,				/* send_raw */
	NULL,				/* roomlist_room_serialize */
	NULL,				/* padding... */
	NULL,
	NULL,
	sizeof(PurplePluginProtocolInfo),    /* struct_size */
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,				     /* magic */
	PURPLE_MAJOR_VERSION,				    /* major_version */
	PURPLE_MINOR_VERSION,				    /* minor_version */
	PURPLE_PLUGIN_PROTOCOL,				  /* type */
	NULL,						    /* ui_requirement */
	0,						       /* flags */
	NULL,						    /* dependencies */
	PURPLE_PRIORITY_DEFAULT,				 /* priority */
	DICT_PROTOCOL_ID,					     /* id */
	"Dictionary protocol",					      /* name */
	"0.1",						   /* version */
	"Dictionary protocol",				  /* summary */
	"Dictionary protocol",				  /* description */
	"Jan Kaluza <hanzz.k@gmail.com>",		     /* author */
	"http://spectrum.im",  /* homepage */
	NULL,						    /* load */
	NULL,						    /* unload */
	NULL,					/* destroy */
	NULL,						    /* ui_info */
	&prpl_info,					      /* extra_info */
	NULL,						    /* prefs_info */
	NULL,					/* actions */
	NULL,						    /* padding... */
	NULL,
	NULL,
	NULL,
};

PURPLE_INIT_PLUGIN(null, dict_init, info);
