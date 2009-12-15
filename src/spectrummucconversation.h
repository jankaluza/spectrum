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

// Wrapper for PurpleConversation.
class SpectrumMUCConversation : public AbstractConversation {
	public:
		SpectrumMUCConversation(PurpleConversation *conv, const std::string &jid);
		~SpectrumMUCConversation();

		void handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);
		void addUsers(AbstractUser *user, GList *cbuddies);
		void renameUser(AbstractUser *user, const char *old_name, const char *new_name, const char *new_alias);
		void removeUsers(AbstractUser *user, GList *users);
		PurpleConversation *getConv() { return m_conv; }
		
	private:
		PurpleConversation *m_conv;
		std::string m_jid;
};

#endif
