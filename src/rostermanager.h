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
#include "abstractuser.h"

using namespace gloox;

class AbstractSpectrumBuddy;
class SpectrumTimer;

// Manages all SpectrumBuddies in user's roster.
class RosterManager {
	public:
		RosterManager(AbstractUser *user);
		~RosterManager();

		// Sends unavailable presence of all online buddies.
		void sendUnavailablePresenceToAll();
		
		// Sends current presence of all buddies.
		void sendPresenceToAll(const std::string &to);
		
		// Remove buddy from local roster.
		void removeFromLocalRoster(const std::string &uin);
		
		// Returns buddy with name `uin`.
		AbstractSpectrumBuddy *getRosterItem(const std::string &uin);
		
		// Adds new buddy to roster.
		void addRosterItem(PurpleBuddy *buddy);
		
		// Sets roster. RosterManager will free it by itself.
		void setRoster(GHashTable *roster);
		
		// Returns true if buddy with this name and subscription is in roster.
		bool isInRoster(const std::string &name, const std::string &subscription);

		// Sends current presence of buddy `s_buddy`.
		void sendPresence(AbstractSpectrumBuddy *s_buddy);

		// Called when buddy signed on.
		void handleBuddySignedOn(AbstractSpectrumBuddy *s_buddy);
		void handleBuddySignedOn(PurpleBuddy *buddy);

		// Called when buddy signed off.
		void handleBuddySignedOff(AbstractSpectrumBuddy *s_buddy);
		void handleBuddySignedOff(PurpleBuddy *buddy);

		// Called when buddy's status changed.
		void handleBuddyStatusChanged(AbstractSpectrumBuddy *s_buddy, PurpleStatus *status, PurpleStatus *old_status);
		void handleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status);

		// Called when buddy is created.
		void handleBuddyCreated(AbstractSpectrumBuddy *s_buddy);

		// Called when buddy is removed
		void handleBuddyRemoved(AbstractSpectrumBuddy *buddy);
		void handleBuddyRemoved(PurpleBuddy *buddy);

		// Checks if there are new buddies in queue and sends them to user.
		// Do not call this function by yourself.
		bool syncBuddiesCallback();

		// Sends buddies from subscribeCache to end user.
		void sendNewBuddies();

	private:
		GHashTable *m_roster;
		AbstractUser *m_user;
		SpectrumTimer *m_syncTimer;
		std::map <std::string, AbstractSpectrumBuddy *> m_subscribeCache;
		int m_subscribeLastCount;

};

#endif
