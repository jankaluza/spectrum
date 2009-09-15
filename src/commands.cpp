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

#include "commands.h"
#include <iostream>
#include <sstream>
#include <string>
#include "cmds.h"
#include "user.h"
#include "usermanager.h"

static PurpleCmdRet settings_command_cb(PurpleConversation *conv, const char *cmd, char **args, char **error, void *data) {
	GString *s = NULL;
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(purple_conversation_get_account(conv));
	if (!user)
		return PURPLE_CMD_RET_OK;
	if (args[0] != NULL) {
		std::string cmd(args[0]);
		if (cmd == "list") {
			s = g_string_new("Transport: ");
			GHashTable *settings = user->settings();
			GHashTableIter iter;
			gpointer key, v;
			g_hash_table_iter_init (&iter, settings);
			while (g_hash_table_iter_next (&iter, &key, &v)) {
				PurpleValue *value = (PurpleValue *) v;
				if (purple_value_get_type(value) == PURPLE_TYPE_BOOLEAN) {
					if (purple_value_get_boolean(value))
						g_string_append_printf(s, "%s = True \n", (char *) key);
					else
						g_string_append_printf(s, "%s = False \n", (char *) key);
				}
				else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
					g_string_append_printf(s, "%s = %s \n", (char *) key, purple_value_get_string(value));
				}
			}
		}
		else if (cmd == "set") {
			if (args[1] != NULL) {
				s = g_string_new("Transport: ");
				PurpleValue *value = user->getSetting(args[1]);
				if (value) {
					if (purple_value_get_type(value) == PURPLE_TYPE_BOOLEAN) {
						value = purple_value_new(PURPLE_TYPE_BOOLEAN);
						purple_value_set_boolean(value, atoi(args[2]));
						user->updateSetting(args[1], value);
						if (purple_value_get_boolean(value))
							g_string_append_printf(s, "%s is now True \n", args[1]);
						else
							g_string_append_printf(s, "%s is now False \n", args[1]);
					}
					else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
						value = purple_value_new(PURPLE_TYPE_STRING);
						purple_value_set_string(value, args[2]);
						user->updateSetting(args[1], value);
						g_string_append_printf(s, "%s is now \"%s\" \n", args[1], args[2]);
					}
				}
				else {
					g_string_append_printf(s, " This setting key doesn't exist. Try to use \"/transport settings list\" to get settings keys.\n");
				}
			}
		}
	}
	if (s) {
		if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
			purple_conv_im_write(PURPLE_CONV_IM(conv), purple_conversation_get_name(conv), s->str, PURPLE_MESSAGE_RECV, time(NULL));
		}
		else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
			purple_conv_chat_write(PURPLE_CONV_CHAT(conv), "transport", s->str, PURPLE_MESSAGE_RECV, time(NULL));
		}
		g_string_free(s, TRUE);
	}

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet help_command_cb(PurpleConversation *conv, const char *cmd, char **args, char **error, void *data) {
	GList *l, *text;
	GString *s;
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(purple_conversation_get_account(conv));
	if (!user)
		return PURPLE_CMD_RET_OK;

	if (args[0] != NULL) {
		s = g_string_new("Transport: ");
		text = purple_cmd_help(conv, args[0]);

		if (text) {
			for (l = text; l; l = l->next)
				if (l->next)
					g_string_append_printf(s, "%s\n", tr(user->getLang(),(char *)l->data));
				else
					g_string_append_printf(s, "%s", tr(user->getLang(),(char *)l->data));
		} else {
			g_string_append(s, tr(user->getLang(),_("No such command (in this context).")));
		}
	} else {
		s = g_string_new(tr(user->getLang(),_("Use \"/transport help &lt;command&gt;\" for help on a specific command.\n"
											 "The following commands are available in this context:\n")));

		text = purple_cmd_list(conv);
		for (l = text; l; l = l->next)
			if (l->next)
				g_string_append_printf(s, "%s, ", tr(user->getLang(),(char *)l->data));
			else
				g_string_append_printf(s, "%s.", tr(user->getLang(),(char *)l->data));
		g_list_free(text);
	}

	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		purple_conv_im_write(PURPLE_CONV_IM(conv), purple_conversation_get_name(conv), s->str, PURPLE_MESSAGE_RECV, time(NULL));
	}
	else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
		purple_conv_chat_write(PURPLE_CONV_CHAT(conv), "transport", s->str, PURPLE_MESSAGE_RECV, time(NULL));
	}
	g_string_free(s, TRUE);

	return PURPLE_CMD_RET_OK;
}

void purple_commands_init() {
	purple_cmd_register("help", "w", PURPLE_CMD_P_DEFAULT, (PurpleCmdFlag) (PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS), NULL, help_command_cb, _("help &lt;command&gt;:  Help on a specific command."), NULL);
	purple_cmd_register("settings", "www", PURPLE_CMD_P_DEFAULT, (PurpleCmdFlag) (PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_IM), NULL, settings_command_cb, _("settings set &lt;command&gt; settings list:  Configure transport."), NULL);
	purple_cmd_register("settings", "w", PURPLE_CMD_P_DEFAULT, (PurpleCmdFlag) (PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_IM), NULL, settings_command_cb, _("settings set &lt;command&gt; settings list:  Configure transport."), NULL);
}
