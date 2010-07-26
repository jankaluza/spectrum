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

#ifndef _ABSTRACT_BACKEND_H
#define _ABSTRACT_BACKEND_H

#include <string>
#include <list>
#include "account.h"
#include "value.h"
#include <vector>

struct UserRow {
	long id;
	std::string jid;
	std::string uin;
	std::string password;
	std::string language;
	std::string encoding;
	bool vip;
};

// Abstract storage backend.
class AbstractBackend {
	public:
		virtual ~AbstractBackend() {}
		virtual void addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type) = 0;
		virtual long addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group = "Buddies", const std::string &nickname = "", int flags = 0) = 0;
		virtual GHashTable *getBuddies(long userId, PurpleAccount *account) = 0;
		virtual std::list <std::string> getBuddies(long userId) = 0;
		virtual void updateBuddySubscription(long userId, const std::string &uin, const std::string &subscription) = 0;
		virtual void removeBuddy(long userId, const std::string &uin, long buddy_id) = 0;
		virtual std::vector<std::string> getOnlineUsers() = 0;
		virtual void setUserOnline(long userId, bool online) = 0;
		virtual UserRow getUserByJid(const std::string &jid) = 0;
		virtual void addUser(const UserRow &user) = 0;
		virtual void removeUser(long userId) = 0;
		virtual void updateUser(const UserRow &user) = 0;
		virtual std::map<std::string, UserRow> getUsersByJid(const std::string &jid) = 0;
		virtual void updateSetting(long userId, const std::string &key, const std::string &value) = 0;
		virtual bool reconnect() { return false; }

};

#endif
