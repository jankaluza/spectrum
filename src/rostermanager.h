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

#ifndef SPECTRUM_ROSTERMANAGER_H
#define SPECTRUM_ROSTERMANAGER_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>

using namespace gloox;

class SpectrumBuddy;
class GlooxMessageHandler;
class User;
struct RosterRow;

class RosterManager {
	public:
		RosterManager(User *user);
		~RosterManager();
		
		void sendUnavailablePresenceToAll();
		void sendPresenceToAll(const std::string &to);
		void removeFromLocalRoster(const std::string &uin);
		Tag *generatePresenceStanza(PurpleBuddy *buddy);
		SpectrumBuddy *getRosterItem(const std::string &uin);
		void addRosterItem(PurpleBuddy *buddy);
		void loadBuddies();
		bool isInRoster(const std::string &name, const std::string &subscription);

	private:
		GHashTable *m_roster;
		User *m_user;

};

#endif
