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
#include "transport_config.h"
#include <algorithm>

#include <gloox/clientbase.h>
#include <gloox/tag.h>
#include <glib.h>
#include "purple.h"
#include <account.h>
#include "abstractbackend.h"

#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/RecordSet.h"

#ifdef WIN32
#define WITH_SQLITE 1
#endif

#ifdef WITH_SQLITE
#include "Poco/Data/SQLite/Connector.h" 
#endif // WITH_SQLITE

#ifdef WITH_MYSQL
#include "Poco/Data/MySQL/Connector.h"
#endif // WITH_MYSQL
#include "spectrumtimer.h"

class GlooxMessageHandler;

using namespace Poco::Data;
using namespace gloox;

#define STATEMENT(STRING) *m_sess << std::string(STRING)

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
	std::string encoding;
	bool vip;
	Poco::Data::Statement *stmt;
};

// Prepared SQL Statement representation
// Usage:
// SpectrumSQLStatement *statement = new SpectrumSQLStatement(session, "s|is", "SELECT id, jid FROM table WHERE user_jid = ?;");
// *statement << "test@domain.tld";
// if (statement->execute()) {
//    *statement >> id_variable >> jid_variable;
// }
class SpectrumSQLStatement {
	public:
		// Creates new SpectrumSQLStatement using m_sess Session.
		// Format is string where each character represents the type of variables used/returned by prepared
		// statement:
		// 'i' - Poco::Int32 (integer)
		// 'I' - std::vector<Poco::Int32>
		// 's' - std::string
		// 'S' - std::vector<std::string>
		// 'b' - boolean
		// '|' - delimiter used to separate input variables from output variables
		// Example statement: "SELECT id, jid FROM table WHERE user_jid = ?;"
		// Format: "s|is" (input: string; output: integer, string)
		SpectrumSQLStatement(Poco::Data::Session *m_sess, const std::string &format, const std::string &statement);
		~SpectrumSQLStatement();

		template <typename T>
		SpectrumSQLStatement& operator << (const T& t);

		template <typename T>
		SpectrumSQLStatement& operator >> (T& t);

		// Executes the statement. All input variables has to have their values pushed before calling
		// execute();
		int execute();
		
		int executeNoCheck();
		void createData();
		void removeData();

		void createStatement(Poco::Data::Session *m_sess, const std::string &statement = "");
		void removeStatement();

	private:
		Poco::Data::Statement *m_statement;
		std::vector <void *> m_params;
		std::string m_format;
		int m_resultOffset;
		int m_offset;
		int m_error;
		std::string m_stmt;
		Poco::Data::Session *m_sess;
};

/*
 * SQL storage backend. Uses libdbi for communication with SQL servers.
 */
class SQLClass : public AbstractBackend {
	public:
		SQLClass(GlooxMessageHandler *parent, bool upgrade = false, bool check = false);
		~SQLClass();

		void addUser(const UserRow &user);
		void addDownload(const std::string &filename, const std::string &vip);
		void removeUser(long userId);
		void updateUser(const UserRow &user);
		void removeUserBuddies(long userId);
		long addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group = "Buddies", const std::string &nickname = "", int flags = 0);
		void updateBuddySubscription(long userId, const std::string &uin, const std::string &subscription);
		void removeBuddy(long userId, const std::string &uin, long buddy_id);
		long getRegisteredUsersCount();
		long getRegisteredUsersRosterCount();
		void createStatements();
		void removeStatements();
		bool reconnect();
		bool ping();
		bool reconnectCallback();
		void upgradeDatabase();

		// settings
		void addSetting(long userId, const std::string &key, const std::string &value, PurpleType type);
		void updateSetting(long userId, const std::string &key, const std::string &value);
		GHashTable * getSettings(long userId);
		void createStatement(SpectrumSQLStatement **statement, const std::string &format, const std::string &sql);
		
		// buddy settings
		void addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type);

		UserRow getUserByJid(const std::string &jid);
		std::map<std::string, UserRow> getUsersByJid(const std::string &jid);
		GHashTable *getBuddies(long userId, PurpleAccount *account);
		std::list <std::string> getBuddies(long userId);
		bool loaded() { return m_loaded; }
		std::vector<std::string> getOnlineUsers();
		void setUserOnline(long userId, bool online);
		void beginTransaction();
		void commitTransaction();
		
	private:
		/*
		 * Creates tables for sqlite3 DB.
		 */
		void initDb();
		SpectrumSQLStatement *m_stmt_addUser;
		SpectrumSQLStatement *m_stmt_updateUserPassword;
		SpectrumSQLStatement *m_stmt_removeBuddy;
		SpectrumSQLStatement *m_stmt_removeUser;
		SpectrumSQLStatement *m_stmt_removeUserBuddies;
		SpectrumSQLStatement *m_stmt_addBuddy;
		SpectrumSQLStatement *m_stmt_updateBuddy;
		SpectrumSQLStatement *m_stmt_updateBuddySubscription;
		SpectrumSQLStatement *m_stmt_getUserByJid;
		SpectrumSQLStatement *m_stmt_addSetting;
		SpectrumSQLStatement *m_stmt_getBuddies;
		SpectrumSQLStatement *m_stmt_updateSetting;
		SpectrumSQLStatement *m_stmt_getBuddiesSettings;
		SpectrumSQLStatement *m_stmt_getSettings;
		SpectrumSQLStatement *m_stmt_addBuddySetting;
		SpectrumSQLStatement *m_stmt_removeBuddySettings;
		SpectrumSQLStatement *m_stmt_setUserOnline;
		SpectrumSQLStatement *m_stmt_getOnlineUsers;
		Poco::Data::Statement *m_version_stmt;

		Poco::Data::Session *m_sess;
		Poco::UInt64 m_version;
		GlooxMessageHandler *p;
		bool m_upgrade;
		bool m_loaded;
		int m_error;
		SpectrumTimer *m_reconnectTimer;
		SpectrumTimer *m_pingTimer;
		int m_dbversion;
		bool m_check;
};

#endif
