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

#ifndef SPECTRUM_BUDDY_H
#define SPECTRUM_BUDDY_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>
#include "abstractspectrumbuddy.h"

using namespace gloox;

// Wrapper for PurpleBuddy
class SpectrumBuddy : public AbstractSpectrumBuddy {
	public:
		SpectrumBuddy(long id, PurpleBuddy *buddy);
		virtual ~SpectrumBuddy();

		// Returns buddy's name (so for example UIN for ICQ, JID for XMPP...).
		std::string getName();

		// Returns buddy's alias (nickname).
		std::string getAlias();

		// Stores current status in `status` and current status message in `statusMessage`.
		// Returns true if data can be stored.
		bool getStatus(PurpleStatusPrimitive &status, std::string &statusMessage);

		// Stores current mood in 'mood' and comment in 'comment'.
		// Returns true if mood is set.
		bool getXStatus(std::string &mood, std::string &comment);

		// Returns SHA-1 hash of buddy icon (avatar) or empty string if there is no avatar.
		std::string getIconHash();

		// Returns buddy's group name.
		std::string getGroup();

		// Returns name which doesn't contain unsafe characters, so it can be used.
		// in JIDs.
		std::string getSafeName();

		// Returns PurpleBuddy* connected with this SpectrumBuddy.
		PurpleBuddy *getBuddy() { return m_buddy; }

		// Changes groups this buddy is in. Note that 'groups' can be changed during
		// this call.
		void changeGroup(std::list<std::string> &groups);

		// Changes server-side alias for this buddy.
		void changeAlias(const std::string &alias);

		void handleBuddyRemoved(PurpleBuddy *buddy);

	private:
		PurpleBuddy *m_buddy;
};

#endif
