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

#ifndef SPECTRUM_MUCCONVERSATION_H
#define SPECTRUM_MUCCONVERSATION_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>
#include "abstractconversation.h"

using namespace gloox;

// Class representing MUC Conversation.
class SpectrumMUCConversation : public AbstractConversation {
	public:
		SpectrumMUCConversation(PurpleConversation *conv, const std::string &jid, const std::string &resource);
		virtual ~SpectrumMUCConversation();

		// Handles message which should be resend to XMPP user.
		void handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);

		// Called when new users join the room.
		void addUsers(AbstractUser *user, GList *cbuddies);

		// Called when some user is renamed.
		void renameUser(AbstractUser *user, const char *old_name, const char *new_name, const char *new_alias);

		// Called when some user is removed.
		void removeUsers(AbstractUser *user, GList *users);

		// Called when some the topic is changed.
		void changeTopic(AbstractUser *user, const char *who, const char *topic);

		// Returns pointer to PurpleConversation associated with this conversation.
		PurpleConversation *getConv() { return m_conv; }

		// Sends current topic to XMPP user.
		void sendTopic(AbstractUser *user);

	private:
		PurpleConversation *m_conv;		// Conversation associated with this class.
		Tag *m_lastPresence;			// Last presence from XMPP user.
		std::string m_nickname;			// XMPP user nickname.
		std::string m_jid;				// Bare JID of the room (e.g. #room%server@irc.localhost).
		bool m_connected;				// True if addUsers has been called.
		std::string m_topic;			// Current topic.
		std::string m_topicUser;		// Name of user who set topic.
		std::string m_resource;			// Current XMPP user's resource.
};

#endif
