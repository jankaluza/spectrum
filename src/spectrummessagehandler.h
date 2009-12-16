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

#ifndef SPECTRUM_MESSAGEHANDLER_H
#define SPECTRUM_MESSAGEHANDLER_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include "gloox/presence.h"
#include "gloox/message.h"
#include <algorithm>
#include "abstractuser.h"
#include "abstractconversation.h"

using namespace gloox;

struct Conversation {
	PurpleConversation *conv;
	std::string resource;
};

// Handles messages.
class SpectrumMessageHandler {
	public:
		SpectrumMessageHandler(AbstractUser *user);
		~SpectrumMessageHandler();

		// Adds new conversation to database.
		void addConversation(PurpleConversation *conv, AbstractConversation *s_conv, const std::string &key = "");

		// Removes and destroyes conversation.
		void removeConversation(const std::string &name);

		// Returns true if the conversation with name exists.
		bool isOpenedConversation(const std::string &name);

		// Removes resource from conversations. New messages will not be sent to this resource anymore.
		void removeConversationResource(const std::string &resource);

		// Returns true if there is some groupchat conversation opened.
		bool hasOpenedMUC();

		// Called by libpurple when new message for non-existing Conversation has been received.
		void handlePurpleMessage(PurpleAccount* account, char * name, char *msg, PurpleConversation *conv, PurpleMessageFlags flags);

		// Handles messages from Jabber side.
		void handleMessage(const Message& msg);

		// Called by libpurple when there is new IM message to be written to Conversation.
		void handleWriteIM(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);

		// Called by libpurple when there is new groupchat message to be written to Conversation.
		void handleWriteChat(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);

		// Called when buddy in room is renamed.
		void purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias);

		// Called when buddy leaves the room.
		void purpleChatRemoveUsers(PurpleConversation *conv, GList *users);

		// Called when new buddies comes to room.
		void purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals);

		// Called when topic is changed.
		void purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic);

	private:
		std::string getConversationName(PurpleConversation *conv);

		std::map <std::string, AbstractConversation *> m_conversations;
		AbstractUser *m_user;
		int m_mucs;
};

#endif
