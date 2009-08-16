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

#ifndef _HI_MUCHANDLER_H
#define _HI_MUCHANDLER_H

#include "localization.h"
#include "gloox/tag.h"
#include "gloox/presence.h"
#include "conversation.h"

class User;
extern Localization localization;

using namespace gloox;

class MUCHandler
{
	public:
		MUCHandler(User *user,const std::string &jid, const std::string &userJid);
		~MUCHandler();
		Tag * handlePresence(const Presence &stanza);
		Tag * handlePresence(Tag *stanza);
		void addUsers(GList *cbuddies);
		void messageReceived(const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);
		void renameUser(const char *old_name, const char *new_name, const char *new_alias);
		void removeUsers(GList *users);
		void topicChanged(const char *who, const char *topic);
		bool isConnected() { return m_connected; }
		void setConversation(PurpleConversation *conv) { m_conv = conv; }

	private:
		void sendTopic();

		User *m_user;
		std::string m_jid;
		std::string m_userJid;
		bool m_connected;
		PurpleConversation *m_conv;
		std::string m_topic;
		std::string m_topicUser;
};

#endif
