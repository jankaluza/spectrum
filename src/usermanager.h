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

#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <string>
#include "purple.h"
#include "account.h"
#include "abstractuser.h"
#include "glib.h"
#include "messagesender.h"

class GlooxMessageHandler;

// Class for managing online XMPP users.
class UserManager : MessageSender
{
	public:
		UserManager();
		~UserManager();

		// Returns user by JID.
		AbstractUser *getUserByJID(std::string barejid);

		// Returns user by PurpleAccount.
		AbstractUser *getUserByAccount(PurpleAccount *account);

		// Adds new user to manager.
		void addUser(AbstractUser *user);

		// Removes user immediately.
		void removeUser(AbstractUser *user);

		// Removes user from GLib's event loop. (setups timer with 0 timeout).
		void removeUserTimer(AbstractUser *user);

		// Removes all users.
		void removeAllUsers();

		// Sets buddy online.
		void buddyOnline();

		// Sets buddy offline.
		void buddyOffline();

		// Returns count of online buddies.
		long onlineBuddiesCount();

		// Returns count of online users;
		int userCount();

		// Sends message to all online users.
		bool sendMessageToAll(const std::string &message);

	private:
		GHashTable *m_users;	// key = JID; value = User*
		long m_onlineBuddies;
		AbstractUser *m_cachedUser;

};

#endif
