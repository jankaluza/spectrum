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

#ifndef SPECTRUM_CONVERSATION_H
#define SPECTRUM_CONVERSATION_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>
#include "abstractconversation.h"

using namespace gloox;

// Class representing IM Conversation
class SpectrumConversation : public AbstractConversation {
	public:
		SpectrumConversation(PurpleConversation *conv, SpectrumConversationType type);
		virtual ~SpectrumConversation();

		// Handles message which should be resend to XMPP user.
		void handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime, const std::string &currentBody = "");

		// Returns pointer to PurpleConversation associated with this conversation.
		PurpleConversation *getConv() { return m_conv; }

	private:
		PurpleConversation *m_conv;		// Conversation associated with this class.
};

#endif
