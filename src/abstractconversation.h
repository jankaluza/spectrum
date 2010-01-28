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

#ifndef ABSTRACT_CONVERSATION_H
#define ABSTRACT_CONVERSATION_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>
#include "abstractuser.h"

typedef enum {	SPECTRUM_CONV_CHAT = 0,
				SPECTRUM_CONV_GROUPCHAT,
				SPECTRUM_CONV_PM
				} SpectrumConversationType;

using namespace gloox;

// Wrapper for PurpleBuddy.
class AbstractConversation {
	public:
		AbstractConversation(SpectrumConversationType type) { m_type = type; m_resource = ""; }
		AbstractConversation() {};

		// Sets the resource associated with this conversation.
		void setResource(const std::string &resource);

		// Returns current resource.
		const std::string &getResource();

		// Returns type of conversation.
		SpectrumConversationType &getType();

		// Handles message which should be resend to XMPP user.
		virtual void handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime, const std::string &currentBody = "") = 0;

		// Called when new users join the room.
		virtual void addUsers(AbstractUser *user, GList *cbuddies) {}

		// Called when some user is renamed.
		virtual void renameUser(AbstractUser *user, const char *old_name, const char *new_name, const char *new_alias) {}

		// Called when some user is removed.
		virtual void removeUsers(AbstractUser *user, GList *users) {}

		// Called when some the topic is changed.
		virtual void changeTopic(AbstractUser *user, const char *who, const char *topic) {}

		// Returns pointer to PurpleConversation associated with this conversation.
		virtual PurpleConversation *getConv() = 0;
	
	private:
		std::string m_resource;				// Current resource.
		SpectrumConversationType m_type;	// Conversation type.
};

#endif
