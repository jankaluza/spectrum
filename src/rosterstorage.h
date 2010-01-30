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

#ifndef SPECTRUM_ROSTERSTORAGE_H
#define SPECTRUM_ROSTERSTORAGE_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>
#include "abstractuser.h"
#include "abstractspectrumbuddy.h"

using namespace gloox;

class SpectrumTimer;

struct SaveData {
	AbstractUser *user;
	long id;
};

// Stores buddies into DB Backend.
class RosterStorage {
	public:
		RosterStorage(AbstractUser *user);
		virtual ~RosterStorage();

		// Add buddy to store queue and store it in future. Nothing
		// will happen if buddy is already added.
		void storeBuddy(PurpleBuddy *buddy);
		void storeBuddy(AbstractSpectrumBuddy *buddy);

		// Store all buddies from queue immediately. Returns true
		// if some buddies were stored.
		bool storeBuddies();

		// Remove buddy from storage queue.
		void removeBuddy(PurpleBuddy *buddy);
		void removeBuddy(AbstractSpectrumBuddy *buddy);

	private:
		AbstractUser *m_user;
		GHashTable *m_storageCache;
		SpectrumTimer *m_storageTimer;
};

#endif
