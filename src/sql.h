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

#ifndef _HI_SQL_H
#define _HI_SQL_H

#include <iostream>

#include <gloox/clientbase.h>
#include <gloox/tag.h>
#include <glib.h>
#include "purple.h"
#include <account.h>

#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/SQLite/Connector.h" 
#include "Poco/Data/MySQL/Connector.h" 

class GlooxMessageHandler;

using namespace Poco::Data;
using namespace gloox;

#define STATEMENT(STRING) *m_sess << std::string(STRING)

/*
 * Structure which represents XMPP User
 */
struct UserRow {
	long id;
	std::string jid;
	std::string uin;
	std::string password;
	std::string language;
};

/*
 * Structure which represents one buddy in roster (one roster row)
 */
struct RosterRow {
	long id;
	std::string jid;
	std::string uin;
	std::string subscription;
	bool online;
	std::string nickname;
	std::string group;
	std::string lastPresence;
};

struct GlooxVCard {
	Tag *vcard;
	time_t created;
};

/*
 * Prepared statements
 */
struct addUserStatement {
	std::string jid;
	std::string uin;
	std::string password;
	std::string language;
	Poco::Data::Statement *stmt;
};

struct updateUserPasswordStatement {
	std::string jid;
	std::string uin;
	std::string password;
	std::string language;
	Poco::Data::Statement *stmt;
};

struct RemoveBuddyStatement {
	Poco::Int32 user_id;
	std::string uin;
	Poco::Data::Statement *stmt;
};

struct RemoveUserStatement {
	std::string jid;
	Poco::Data::Statement *stmt;
};

struct RemoveUserBuddiesStatement {
	Poco::Int32 user_id;
	Poco::Data::Statement *stmt;
};

struct AddBuddyStatement {
	Poco::Int32 user_id;
	std::string uin;
	std::string subscription;
	std::string groups;
	std::string nickname;
	Poco::Data::Statement *stmt;
};

struct updateBuddySubscriptionStatement {
	Poco::Int32 user_id;
	std::string uin;
	std::string subscription;
	std::string groups;
	std::string nickname;
	Poco::Data::Statement *stmt;
};

struct getUserByJidStatement {
	std::string jid;
	Poco::Data::Statement *stmt;
	
	Poco::Int32 resId;
	std::string resJid;
	std::string resUin;
	std::string resPassword;
};

struct getBuddiesStatement {
	Poco::Int32 user_id;
	Poco::Data::Statement *stmt;
	
	Poco::Int32 resId;
	Poco::Int32 resUserId;
	std::string resUin;
	std::string resSubscription;
	std::string resNickname;
	std::string resGroups;
};

struct addSettingStatement {
	Poco::Int32 user_id;
	std::string var;
	std::string value;
	int type;
	Poco::Data::Statement *stmt;
};

struct updateSettingStatement {
	Poco::Int32 user_id;
	std::string var;
	std::string value;
	Poco::Data::Statement *stmt;
};

struct getBuddiesSettingsStatement {
	Poco::Int32 user_id;
	Poco::Data::Statement *stmt;
	
	std::vector<Poco::Int32> resId; 
	std::vector<int> resType;
	std::vector<std::string> resVar;
	std::vector<std::string> resValue;
};

struct getSettingsStatement {
	Poco::Int32 user_id;
	Poco::Data::Statement *stmt;
	
	std::vector<Poco::Int32> resId; 
	std::vector<int> resType;
	std::vector<std::string> resVar;
	std::vector<std::string> resValue;
};

struct addBuddySettingStatement {
	Poco::Int32 user_id;
	Poco::Int32 buddy_id;
	std::string var;
	int type;
	std::string value;
	Poco::Data::Statement *stmt;
};

/*
 * SQL storage backend. Uses libdbi for communication with SQL servers.
 */
class SQLClass {
	public:
		SQLClass(GlooxMessageHandler *parent);
		~SQLClass();

		void addUser(const std::string &jid, const std::string &uin, const std::string &password, const std::string &language);
		void addDownload(const std::string &filename, const std::string &vip);
		void removeUser(const std::string &jid);
		void updateUserPassword(const std::string &jid,const std::string &password, const std::string &language);
		void removeUserBuddies(long userId);
		long addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group = "Buddies", const std::string &nickname = "");
	// 	void updateUserToRoster(const std::string &jid,const std::string &uin,const std::string &subscription, const std::string &group = "Buddies", const std::string &nickname = "");
		void updateBuddySubscription(long userId, const std::string &uin, const std::string &subscription);
		void removeBuddy(long userId, const std::string &uin);
		bool isVIP(const std::string &jid); // TODO: remove me, I'm not needed with new db schema
		long getRegisteredUsersCount();
		long getRegisteredUsersRosterCount();

		// settings
		void addSetting(long userId, const std::string &key, const std::string &value, PurpleType type);
		void updateSetting(long userId, const std::string &key, const std::string &value);
		GHashTable * getSettings(long userId);
		
		// buddy settings
		void addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type);

		UserRow getUserByJid(const std::string &jid);
		std::map<std::string,RosterRow> getBuddies(long userId, PurpleAccount *account = NULL);
		bool loaded() { return m_loaded; }
		
	private:
		/*
		 * Creates tables for sqlite3 DB.
		 */
		void initDb();
		addUserStatement m_stmt_addUser;
		updateUserPasswordStatement m_stmt_updateUserPassword;
		RemoveBuddyStatement m_stmt_removeBuddy;
		RemoveUserStatement m_stmt_removeUser;
		RemoveUserBuddiesStatement m_stmt_removeUserBuddies;
		AddBuddyStatement m_stmt_addBuddy;
		updateBuddySubscriptionStatement m_stmt_updateBuddySubscription;
		getUserByJidStatement m_stmt_getUserByJid;
		getBuddiesStatement m_stmt_getBuddies;
		addSettingStatement m_stmt_addSetting;
		updateSettingStatement m_stmt_updateSetting;
		getBuddiesSettingsStatement m_stmt_getBuddiesSettings;
		getSettingsStatement m_stmt_getSettings;
		addBuddySettingStatement m_stmt_addBuddySetting;

		Poco::Data::Session *m_sess;
		GlooxMessageHandler *p;
		bool m_loaded;
};

#endif
