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
#include "user.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>

using namespace gloox;

class SpectrumBuddy;
class GlooxMessageHandler;
class SpectrumTimer;

class RosterManager {
	public:
		RosterManager(GlooxMessageHandler *m, User *user);
		~RosterManager();

		bool isInRoster(SpectrumBuddy *buddy);
		
		bool addBuddy(PurpleBuddy *buddy);
		bool addBuddy(SpectrumBuddy *buddy);
		void removeBuddy(SpectrumBuddy *buddy);
		void storeBuddy(PurpleBuddy *);
		void storeBuddy(SpectrumBuddy *);

		SpectrumBuddy *getBuddy(const std::string &name);
		void loadBuddies();

		void handleSubscribed(const std::string &uin, const std::string &from);
		void handleSubscribe(const std::string &uin, const std::string &from);
		void handleUnsubscribe(const std::string &uin, const std::string &from);
		void handleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status);
		void handleBuddySignedOn(PurpleBuddy *buddy);
		void handleBuddySignedOff(PurpleBuddy *buddy);

		
		bool storageTimeout();
		bool subscribeBuddies();

		bool loadingBuddies() { return m_loadingBuddies; }
		User *getUser() { return m_user; }
		GHashTable *storageCache() { return m_storageCache; }

	private:
		GlooxMessageHandler *m_main;
		GHashTable *m_roster;
		GHashTable *m_storageCache;
		User *m_user;
		SpectrumTimer *m_syncTimer;
		SpectrumTimer *m_storageTimer;
		int m_cacheSize;
		int m_oldCacheSize;
		bool m_loadingBuddies;
		
		SpectrumBuddy* purpleBuddyToSpectrumBuddy(PurpleBuddy *buddy, bool create = FALSE);
		static gboolean subscribeBuddiesWrapper(void *rosterManager, void *data);

};

#endif
