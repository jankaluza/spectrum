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

class GlooxMessageHandler;

class UserManager
{
	public:
		UserManager();
		~UserManager();
		AbstractUser *getUserByJID(std::string barejid);
		AbstractUser *getUserByAccount(PurpleAccount *account);
		void addUser(AbstractUser *user) { g_hash_table_replace(m_users, g_strdup(user->userKey().c_str()), user); }
		void removeUser(AbstractUser *user);
		void removeUserTimer(AbstractUser *user);
		void buddyOnline();
		void buddyOffline();
		long onlineUserCount();
		int userCount() { return g_hash_table_size(m_users); }

	private:
		GHashTable *m_users;	// key = JID; value = User*
		long m_onlineBuddies;
		AbstractUser *m_cachedUser;

};

#endif
