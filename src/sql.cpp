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

#include "sql.h"
#include "parser.h"
#include "log.h"
#include "main.h"
#include "spectrumbuddy.h"
#include "spectrum_util.h"
#include "protocols/abstractprotocol.h"
#include "transport.h"
#include "usermanager.h"
#include <sys/time.h>

#define SQLITE_DB_VERSION 3
#define MYSQL_DB_VERSION 2

#if !defined(WITH_MYSQL) && !defined(WITH_SQLITE) && !defined(WITH_ODBC)
#error There is no libPocoData storage backend installed. Spectrum will not work without one of them.
#endif

#ifdef WITH_SQLITE
#include <Poco/Data/SQLite/SQLiteException.h>
#endif

static gboolean reconnectMe(gpointer data) {
	SQLClass *sql = (SQLClass *) data;
	return sql->reconnectCallback();
}

static gboolean pingSQL(gpointer data) {
	SQLClass *sql = (SQLClass *) data;
	return sql->ping();
}

SpectrumSQLStatement::SpectrumSQLStatement(Poco::Data::Session *sess, const std::string &format, const std::string &statement) {
	m_format = format;
	m_statement = NULL;
	m_sess = sess;
	createStatement(statement);
}

SpectrumSQLStatement::~SpectrumSQLStatement() {
	removeStatement();
}

void SpectrumSQLStatement::createStatement(const std::string &stmt) {
	std::string statement;
	if (stmt.empty() && m_statement)
		statement = m_statement->toString();
	else
		statement = stmt;
	if (m_statement) {
		removeStatement();
	}
	m_resultOffset = -1;
	m_offset = 0;
	m_error = 0;
	m_statement = new Statement(*m_sess << statement);

#define BIND(VARIABLE) m_params.push_back(data); \
if (selectPart) *m_statement = (*m_statement), use(*VARIABLE); else *m_statement = (*m_statement), into(*VARIABLE);

	bool selectPart = true;
	void *data;
	for (int i = 0; i < m_format.length(); i++) {
		switch (m_format.at(i)) {
			case 's':
				data = new std::string;
				BIND((std::string *) data);
				break;
			case 'S':
				data = new std::vector<std::string>;
				BIND((std::vector<std::string> *) data);
				break;
			case 'i':
				data = new Poco::Int32;
				BIND((Poco::Int32 *) data);
				break;
			case 'I':
				data = new std::vector<Poco::Int32>;
				BIND((std::vector<Poco::Int32> *) data);
				break;
			case 'b':
				data = new bool;
				BIND((bool *) data);
				break;
			case '|':
				selectPart = false;
				m_resultOffset = i;
				break;
		}
	}
	if (m_resultOffset < 0)
		m_resultOffset =  m_format.size();

#undef BIND
}

void SpectrumSQLStatement::removeStatement() {
	if (m_statement) {
		delete m_statement;
		for (int i = 0; i < m_format.length(); i++) {
			switch (m_format.at(i)) {
				case 's':
					delete (std::string *) m_params.front();
					m_params.erase(m_params.begin());
					break;
				case 'S':
					delete (std::vector <std::string> *) m_params.front();
					m_params.erase(m_params.begin());
					break;
				case 'i':
					delete (Poco::Int32 *) m_params.front();
					m_params.erase(m_params.begin());
					break;
				case 'I':
					delete (std::vector <Poco::Int32> *) m_params.front();
					m_params.erase(m_params.begin());
					break;
				case 'b':
					delete (bool *) m_params.front();
					m_params.erase(m_params.begin());
					break;
				case '|':
					break;
			}
		}
	}
}

int SpectrumSQLStatement::execute() {
	if (m_offset != m_resultOffset) {
		Log("SpectrumSQLStatement::execute()", "ERROR: there are some unpushed variables");
		return 0;
	}

	int ret = 0;
	STATEMENT_EXECUTE_BEGIN();
	ret = m_statement->execute();
	STATEMENT_EXECUTE_END("",execute());
	
	// If statement has some input and doesn't have any output, we have
	// to clear the offset now, because operator>> will not be called.
	if (m_resultOffset != 0 && m_offset + 1 == m_params.size()) {
		m_offset = 0;
	}
	return ret;
}

template <typename T>
SpectrumSQLStatement& SpectrumSQLStatement::operator << (const T& t) {
	if (m_offset >= m_resultOffset)
		return *this;

	T *data = (T *) m_params[m_offset];
	*data = t;
	m_offset++;
	return *this;
}

template <typename T>
SpectrumSQLStatement& SpectrumSQLStatement::operator >> (T& t) {
	std::swap(t, *(T *) m_params[m_offset]);
	if (++m_offset == m_params.size())
		m_offset = 0;
	return *this;
}

SQLClass::SQLClass(GlooxMessageHandler *parent, bool upgrade, bool check) {
	p = parent;
	m_loaded = false;
	m_check = check;
	m_upgrade = upgrade;
	m_sess = NULL;
	m_stmt_addUser.stmt = NULL;
	m_version_stmt = NULL;
	m_stmt_updateUserPassword.stmt = NULL;
	m_stmt_removeBuddy.stmt = NULL;
	m_stmt_removeUser.stmt = NULL;
	m_stmt_removeUserBuddies.stmt = NULL;
	m_stmt_addBuddy.stmt = NULL;
#ifdef WITH_SQLITE
	m_stmt_updateBuddy.stmt = NULL;
#endif
// 	m_stmt_updateBuddySubscription.stmt = NULL;
// 	m_stmt_getBuddies.stmt = NULL;
// 	m_stmt_addSetting.stmt = NULL;
// 	m_stmt_updateSetting.stmt = NULL;
// 	m_stmt_getBuddiesSettings.stmt = NULL;
// 	m_stmt_addBuddySetting.stmt = NULL;
// 	m_stmt_getSettings.stmt = NULL;
// 	m_stmt_getOnlineUsers.stmt = NULL;
// 	m_stmt_setUserOnline.stmt = NULL;
// 	m_stmt_removeBuddySettings.stmt = NULL;
	
	m_error = 0;
	m_reconnectTimer = new SpectrumTimer(1000, reconnectMe, this);
	m_pingTimer = new SpectrumTimer(30000, pingSQL, this);
	
	
	try {
#ifdef WITH_MYSQL
		if (p->configuration().sqlType == "mysql") {
			m_dbversion = MYSQL_DB_VERSION;
			MySQL::Connector::registerConnector();
			m_sess = new Session("MySQL", "user=" + p->configuration().sqlUser + ";password=" + p->configuration().sqlPassword + ";host=" + p->configuration().sqlHost + ";db=" + p->configuration().sqlDb + ";auto-reconnect=true");
			if (!check && !upgrade)
				m_pingTimer->start();
		}
#endif
#ifdef WITH_SQLITE
		if (p->configuration().sqlType == "sqlite") {
			m_dbversion = SQLITE_DB_VERSION;
			SQLite::Connector::registerConnector(); 
			m_sess = new Session("SQLite", p->configuration().sqlDb);
			g_chmod(p->configuration().sqlDb.c_str(), 0640);
		}
#endif
	}
	catch (Poco::Exception e) {
		Log("SQL ERROR", e.displayText());
		return;
	}
	
	if (!m_sess) {
		Log("SQL ERROR", "You don't have Spectrum compiled with the storage backend you are trying to use");
		Log("SQL ERROR", "Currently Spectrum is compiled with these storage backends:");
#ifdef WITH_MYSQL
		Log("SQL ERROR", " - MySQL - use type=mysql for this backend in config file");
#endif
#ifdef WITH_SQLITE
		Log("SQL ERROR", " - SQLite - use type=sqlite for this backend in config file");
#endif
// #ifdef WITH_ODBC
// 		Log().Get("SQL ERROR") << " - ODBC - use type=odbc for this backend in config file";
// #endif
		return;
	}
	
	if (check) {
		std::cout << "Required DB schema version: " << m_dbversion << "\n";
	}
	
	createStatements();
	
	initDb();

// 	if (!vipSQL->connect("platby",p->configuration().sqlHost.c_str(),p->configuration().sqlUser.c_str(),p->configuration().sqlPassword.c_str()))
}

SQLClass::~SQLClass() {
	delete m_reconnectTimer;
	delete m_pingTimer;
	if (m_loaded) {
		m_sess->close();
		delete m_sess;
		removeStatements();
	}
}

void SQLClass::createStatement() {
	createStatements();
}

void SQLClass::createStatements() {
	if (!m_version_stmt) {
		m_version_stmt = new Statement( ( STATEMENT("SELECT ver FROM " + p->configuration().sqlPrefix + "db_version ORDER BY ver DESC LIMIT 1"), into(m_version) ) );
	}
	
	// Prepared statements
#ifdef WITH_SQLITE
	if (p->configuration().sqlType == "sqlite") {
		if (!m_stmt_addUser.stmt)
			m_stmt_addUser.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES (?, ?, ?, ?, ?, DATETIME('NOW'), ?)"),
												use(m_stmt_addUser.jid),
												use(m_stmt_addUser.uin),
												use(m_stmt_addUser.password),
												use(m_stmt_addUser.language),
												use(m_stmt_addUser.encoding),
												use(m_stmt_addUser.vip) ) );
	} else
#endif
		if (!m_stmt_addUser.stmt)
			m_stmt_addUser.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "users (jid, uin, password, language, encoding, last_login, vip) VALUES (?, ?, ?, ?, ?, NOW(), ?)"),
												use(m_stmt_addUser.jid),
												use(m_stmt_addUser.uin),
												use(m_stmt_addUser.password),
												use(m_stmt_addUser.language),
												use(m_stmt_addUser.encoding),
												use(m_stmt_addUser.vip) ) );
	if (!m_stmt_updateUserPassword.stmt)
		m_stmt_updateUserPassword.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "users SET password=?, language=?, encoding=?, vip=? WHERE jid=?"),
														use(m_stmt_updateUserPassword.password),
														use(m_stmt_updateUserPassword.language),
														use(m_stmt_updateUserPassword.encoding),
														use(m_stmt_updateUserPassword.vip),
														use(m_stmt_updateUserPassword.jid) ) );
	if (!m_stmt_removeBuddy.stmt)
		m_stmt_removeBuddy.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? AND uin=?"),
												use(m_stmt_removeBuddy.user_id),
												use(m_stmt_removeBuddy.uin) ) );
	if (!m_stmt_removeUser.stmt)
		m_stmt_removeUser.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "users WHERE id=?"),
												use(m_stmt_removeUser.userId) ) );
	if (!m_stmt_removeUserBuddies.stmt)
		m_stmt_removeUserBuddies.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=?"),
														use(m_stmt_removeUserBuddies.user_id) ) );
#ifdef WITH_SQLITE
	if (p->configuration().sqlType == "sqlite") {
		if (!m_stmt_addBuddy.stmt)
			m_stmt_addBuddy.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?)"),
												use(m_stmt_addBuddy.user_id),
												use(m_stmt_addBuddy.uin),
												use(m_stmt_addBuddy.subscription),
												use(m_stmt_addBuddy.groups),
												use(m_stmt_addBuddy.nickname),
												use(m_stmt_addBuddy.flags)) );
		if (!m_stmt_updateBuddy.stmt)
			m_stmt_updateBuddy.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "buddies SET groups=?, nickname=?, flags=? WHERE user_id=? AND uin=?"),
												use(m_stmt_updateBuddy.groups),
												use(m_stmt_updateBuddy.nickname),
												use(m_stmt_updateBuddy.flags),
												use(m_stmt_updateBuddy.user_id),
												use(m_stmt_updateBuddy.uin) ) );
	} else
#endif
		if (!m_stmt_addBuddy.stmt)
			m_stmt_addBuddy.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies (user_id, uin, subscription, groups, nickname, flags) VALUES (?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE groups=?, nickname=?"),
												use(m_stmt_addBuddy.user_id),
												use(m_stmt_addBuddy.uin),
												use(m_stmt_addBuddy.subscription),
												use(m_stmt_addBuddy.groups),
												use(m_stmt_addBuddy.nickname),
												use(m_stmt_addBuddy.flags),
												use(m_stmt_addBuddy.groups),
												use(m_stmt_addBuddy.nickname) ) );

	m_stmt_updateBuddySubscription = new SpectrumSQLStatement(m_sess, "sis", "UPDATE " + p->configuration().sqlPrefix + "buddies SET subscription=? WHERE user_id=? AND uin=?");
	m_stmt_getUserByJid = new SpectrumSQLStatement(m_sess, "s|isssssb", "SELECT id, jid, uin, password, encoding, language, vip FROM " + p->configuration().sqlPrefix + "users WHERE jid=?");

	m_stmt_getBuddies = new SpectrumSQLStatement(m_sess, "i|IISSSSI", "SELECT id, user_id, uin, subscription, nickname, groups, flags FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? ORDER BY id ASC");

	m_stmt_addSetting = new SpectrumSQLStatement(m_sess, "isis", "INSERT INTO " + p->configuration().sqlPrefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)");
	m_stmt_updateSetting = new SpectrumSQLStatement(m_sess, "sis", "UPDATE " + p->configuration().sqlPrefix + "users_settings SET value=? WHERE user_id=? AND var=?");
	m_stmt_getBuddiesSettings = new SpectrumSQLStatement(m_sess, "i|IISS", "SELECT buddy_id, type, var, value FROM " + p->configuration().sqlPrefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC");
	if (p->configuration().sqlType == "sqlite")
		m_stmt_addBuddySetting = new SpectrumSQLStatement(m_sess, "iisis", "INSERT OR REPLACE INTO " + p->configuration().sqlPrefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)");
	else
		m_stmt_addBuddySetting = new SpectrumSQLStatement(m_sess, "iisiss", "INSERT INTO " + p->configuration().sqlPrefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE value=?");

	m_stmt_removeBuddySettings = new SpectrumSQLStatement(m_sess, "i", "DELETE FROM " + p->configuration().sqlPrefix + "buddies_settings WHERE buddy_id=?");
	m_stmt_getSettings = new SpectrumSQLStatement(m_sess, "i|IISS", "SELECT user_id, type, var, value FROM " + p->configuration().sqlPrefix + "users_settings WHERE user_id=?");
	
	m_stmt_getOnlineUsers= new SpectrumSQLStatement(m_sess, "|S","SELECT jid FROM " + p->configuration().sqlPrefix + "users WHERE online=1");

	if (p->configuration().sqlType == "sqlite") {
		m_stmt_setUserOnline = new SpectrumSQLStatement(m_sess, "bi", "UPDATE " + p->configuration().sqlPrefix + "users SET online=?, last_login=DATETIME('NOW')  WHERE id=?");
	}
	else
		m_stmt_setUserOnline = new SpectrumSQLStatement(m_sess, "bi", "UPDATE " + p->configuration().sqlPrefix + "users SET online=?, last_login=NOW()  WHERE id=?");
}

void SQLClass::addUser(const UserRow &user) {
	m_stmt_addUser.jid.assign(user.jid);
	m_stmt_addUser.uin.assign(user.uin);
	m_stmt_addUser.password.assign(user.password);
	m_stmt_addUser.language.assign(user.language);
	m_stmt_addUser.encoding.assign(user.encoding);
	m_stmt_addUser.vip = user.vip;
	try {
		m_stmt_addUser.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log("SQL ERROR", e.displayText());
	}
}

void SQLClass::removeStatements() {
	delete m_stmt_addUser.stmt;
	delete m_version_stmt;
	delete m_stmt_updateUserPassword.stmt;
	delete m_stmt_removeBuddy.stmt;
	delete m_stmt_removeUser.stmt;
	delete m_stmt_removeUserBuddies.stmt;
	delete m_stmt_addBuddy.stmt;
	delete m_stmt_removeBuddySettings;
#ifdef WITH_SQLITE
	delete m_stmt_updateBuddy.stmt;
#endif
	delete m_stmt_updateBuddySubscription;
	delete m_stmt_getBuddies;
	delete m_stmt_addSetting;
	delete m_stmt_updateSetting;
	delete m_stmt_getUserByJid;
	delete m_stmt_getBuddiesSettings;
	delete m_stmt_addBuddySetting;
	delete m_stmt_getSettings;
	delete m_stmt_getOnlineUsers;
	delete m_stmt_setUserOnline;
	/*
	m_stmt_addUser.stmt = NULL;
	m_version_stmt = NULL;
	m_stmt_updateUserPassword.stmt = NULL;
	m_stmt_removeBuddy.stmt = NULL;
	m_stmt_removeUser.stmt = NULL;
	m_stmt_removeUserBuddies.stmt = NULL;
	m_stmt_addBuddy.stmt = NULL;
#ifdef WITH_SQLITE
	m_stmt_updateBuddy.stmt = NULL;
#endif
	m_stmt_updateBuddySubscription.stmt = NULL;
	m_stmt_getBuddies.stmt = NULL;
	m_stmt_addSetting.stmt = NULL;
	m_stmt_updateSetting.stmt = NULL;
	m_stmt_getBuddiesSettings.stmt = NULL;
	m_stmt_addBuddySetting.stmt = NULL;
	m_stmt_getSettings.stmt = NULL;
	m_stmt_getOnlineUsers.stmt = NULL;
	m_stmt_setUserOnline.stmt = NULL;
	m_stmt_removeBuddySettings.stmt = NULL;*/
}

bool SQLClass::reconnect() {
	int i = 20;
	if (m_loaded) {
		removeStatements();
		m_sess->close();
		delete m_sess;

		// This loop blocks whole transport and tries to reconnect 20x (without db transport just can't work,
		// so that's feature, not bug).
		for (i = 0; i < 20; i++) {
			try {
				m_sess = new Session("MySQL", "user=" + p->configuration().sqlUser + ";password=" + p->configuration().sqlPassword + ";host=" + p->configuration().sqlHost + ";db=" + p->configuration().sqlDb + ";auto-reconnect=true");
				break;
			}
			catch (Poco::Exception e) {
				Log("SQL ERROR", "Can't reconnect to db. Try number " << i + 1);
			}
			g_usleep(G_USEC_PER_SEC);
		}
	}

	if (i == 20) {
		if (m_loaded) {
			m_loaded = false;
			Log("SQL ERROR", "Removing All connected users.");
			Transport::instance()->userManager()->removeAllUsers();
			Log("SQL ERROR", "Disconnecting from Jabber");
			p->j->disconnect();
			m_reconnectTimer->start();
		}
		return false;
	}
	else {
		createStatements();
		m_version_stmt->execute();
		m_loaded = true;
	}
	return true;
}

bool SQLClass::reconnectCallback() {
	try {
		m_sess = new Session("MySQL", "user=" + p->configuration().sqlUser + ";password=" + p->configuration().sqlPassword + ";host=" + p->configuration().sqlHost + ";db=" + p->configuration().sqlDb + ";auto-reconnect=true");
		createStatements();
		m_version_stmt->execute();
		m_loaded = true;
	}
	catch (Poco::Exception e) {
		Log("SQL ERROR", "Can't reconnect to db. Will try it again after 1 second.");
		m_loaded = false;
		return true;
	}

	return false;
}

bool SQLClass::ping() {
#ifdef WITH_MYSQL
	//TODO: better use mysql_ping() but thats not supported by libpoco
	*m_sess <<  "SELECT 1", now;
#endif
	return true;
}

void SQLClass::initDb() {
	if (p->configuration().sqlType == "sqlite") {
		try {
			m_version_stmt->execute();
		}
		catch (Poco::Exception e) {
			try {
				*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "buddies ("
							"  id INTEGER PRIMARY KEY NOT NULL,"
							"  user_id int(10) NOT NULL,"
							"  uin varchar(255) NOT NULL,"
							"  subscription varchar(20) NOT NULL,"
							"  nickname varchar(255) NOT NULL,"
							"  groups varchar(255) NOT NULL,"
							"  flags int(4) NOT NULL DEFAULT '0'"
							");", now;

				*m_sess << "CREATE UNIQUE INDEX IF NOT EXISTS user_id ON " + p->configuration().sqlPrefix + "buddies (user_id, uin);", now;

				*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "buddies_settings ("
							"  user_id int(10) NOT NULL,"
							"  buddy_id int(10) NOT NULL,"
							"  var varchar(50) NOT NULL,"
							"  type int(4) NOT NULL,"
							"  value varchar(255) NOT NULL,"
							"  PRIMARY KEY (buddy_id, var)"
							");", now;

				*m_sess << "CREATE INDEX IF NOT EXISTS user_id02 ON " + p->configuration().sqlPrefix + "buddies_settings (user_id);", now;

				*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "users ("
							"  id INTEGER PRIMARY KEY NOT NULL,"
							"  jid varchar(255) NOT NULL,"
							"  uin varchar(4095) NOT NULL,"
							"  password varchar(255) NOT NULL,"
							"  language varchar(25) NOT NULL,"
							"  encoding varchar(50) NOT NULL DEFAULT 'utf8',"
							"  last_login datetime,"
							"  vip int(1) NOT NULL DEFAULT '0',"
							"  online int(1) NOT NULL DEFAULT '0'"
							");", now;

				*m_sess << "CREATE UNIQUE INDEX IF NOT EXISTS jid ON " + p->configuration().sqlPrefix + "users (jid);", now;

				*m_sess << "CREATE TABLE " + p->configuration().sqlPrefix + "users_settings ("
							"  user_id int(10) NOT NULL,"
							"  var varchar(50) NOT NULL,"
							"  type int(4) NOT NULL,"
							"  value varchar(255) NOT NULL,"
							"  PRIMARY KEY (user_id, var)"
							");", now;
							
				*m_sess << "CREATE INDEX IF NOT EXISTS user_id03 ON " + p->configuration().sqlPrefix + "users_settings (user_id);", now;

				*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "db_version ("
					"  ver INTEGER NOT NULL DEFAULT '3'"
					");", now;
				*m_sess << "REPLACE INTO " + p->configuration().sqlPrefix + "db_version (ver) values(3)", now;
			}
			catch (Poco::Exception e) {
				Log("SQL ERROR", e.displayText());
			}
		}
	}

	try {
		m_version_stmt->execute();
	}
	catch (Poco::Exception e) {
		m_version = 0;
		if (m_check) {
			m_loaded = false;
			std::cout << "Current DB schema version: " << m_version << "\n";
			exit(1);
		}
		else {
			Log("SQL ERROR", e.displayText());
			Log("SQL ERROR CODE", e.code());
			if (p->configuration().sqlType != "sqlite" && !m_upgrade) {
				Log("SQL", "Maybe the database schema is not updated. Try to run \"spectrum <config_file.cfg> --upgrade-db\" to fix that.");
				return;
			}
			if (p->configuration().sqlType == "sqlite") {
				m_loaded = true;
				upgradeDatabase();
				return;
			}
		}
	}
	if (m_check) {
		m_loaded = false;
		std::cout << "Current DB schema version: " << m_version << "\n";
		if (m_version < m_dbversion && p->configuration().sqlType != "sqlite")
			exit(2);
		else
			exit(0);
	}
	else {
		Log("SQL", "Current DB version: " << m_version);
		if ((p->configuration().sqlType == "sqlite" || (p->configuration().sqlType != "sqlite" && m_upgrade)) && m_version < m_dbversion) {
			Log("SQL", "Starting DB upgrade.");
			m_loaded = true;
			upgradeDatabase();
			if (m_upgrade)
				exit(0);
			return;
		}
		else if (m_version < m_dbversion) {
			Log("SQL", "Maybe the database schema is not updated. Try to run \"spectrum <config_file.cfg> --upgrade-db\" to fix that.");
			return;
		}
		else if (m_upgrade) {
			Log("SQL", "Database schema is up to date, you can run spectrum as usual.");
			return;
		}
	}
	
	m_loaded = true;

}

void SQLClass::upgradeDatabase() {
	try {
		for (int i = (int) m_version; i < m_dbversion; i++) {
			Log("SQL", "Upgrading from version " << i << " to " << i + 1);
			if (i == 0) {
				if (p->configuration().sqlType == "sqlite") {
					*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "db_version ("
						"  ver INTEGER NOT NULL DEFAULT '1'"
						");", now;
					*m_sess << "ALTER TABLE " + p->configuration().sqlPrefix + "users ADD online int(1) NOT NULL DEFAULT '0';", now;
				}
				else {
					*m_sess << "CREATE TABLE IF NOT EXISTS `" + p->configuration().sqlPrefix + "db_version` ("
								"`ver` int(10) unsigned NOT NULL default '1'"
								");", now;
					*m_sess << "ALTER TABLE " + p->configuration().sqlPrefix + "users ADD online tinyint(1) NOT NULL DEFAULT '0';", now;
				}
				*m_sess << "REPLACE INTO " + p->configuration().sqlPrefix + "db_version (ver) values(1)", now;
			}
			else if (i == 1) {
				if (p->configuration().sqlType == "sqlite") {
					// Change 'vip' column type from tinyint to int(1)
					*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "users_new ("
								"  id INTEGER PRIMARY KEY NOT NULL,"
								"  jid varchar(255) NOT NULL,"
								"  uin varchar(4095) NOT NULL,"
								"  password varchar(255) NOT NULL,"
								"  language varchar(25) NOT NULL,"
								"  encoding varchar(50) NOT NULL DEFAULT 'utf8',"
								"  last_login datetime,"
								"  vip int(1) NOT NULL DEFAULT '0',"
								"  online int(1) NOT NULL DEFAULT '0'"
								");", now;
					*m_sess << "INSERT INTO " + p->configuration().sqlPrefix + "users_new SELECT id,jid,uin,password,language,encoding,last_login,vip,online FROM " + p->configuration().sqlPrefix + "users;", now;
					*m_sess << "DROP TABLE " + p->configuration().sqlPrefix + "users;", now;
					*m_sess << "ALTER TABLE " + p->configuration().sqlPrefix + "users_new RENAME TO " + p->configuration().sqlPrefix + "users;", now;
					*m_sess << "CREATE UNIQUE INDEX IF NOT EXISTS jid1 ON " + p->configuration().sqlPrefix + "users (jid);", now;
					*m_sess << "REPLACE INTO " + p->configuration().sqlPrefix + "db_version (ver) values(2)", now;
				}
				else {
					// Add 'flags' column to buddies table.
					*m_sess << "ALTER TABLE " + p->configuration().sqlPrefix + "buddies ADD flags smallint(4) NOT NULL DEFAULT '0';", now;
					*m_sess << "REPLACE INTO " + p->configuration().sqlPrefix + "db_version (ver) values(2);", now;
				}
			}
			else if (i == 2) {
				if (p->configuration().sqlType == "sqlite") {
					// Add 'flags' column to buddies table.
					*m_sess << "ALTER TABLE " + p->configuration().sqlPrefix + "buddies ADD flags int(4) NOT NULL DEFAULT '0';", now;
					*m_sess << "REPLACE INTO " + p->configuration().sqlPrefix + "db_version (ver) values(3);", now;
				}
			}
		}
	}
	catch (Poco::Exception e) {
		m_loaded = false;
		Log("SQL ERROR", e.displayText());
		return;
	}
	Log("SQL", "Done");
}

long SQLClass::getRegisteredUsersCount(){
	Poco::UInt64 users;
	*m_sess << "SELECT count(*) FROM " + p->configuration().sqlPrefix + "users", into(users), now;
	return users;
}

long SQLClass::getRegisteredUsersRosterCount(){
	Poco::UInt64 users;
	*m_sess << "SELECT count(*) FROM " + p->configuration().sqlPrefix + "buddies", into(users), now;
	return users;
}

void SQLClass::updateUser(const UserRow &user) {
	m_stmt_updateUserPassword.jid.assign(user.jid);
	m_stmt_updateUserPassword.password.assign(user.password);
	m_stmt_updateUserPassword.language.assign(user.language);
	m_stmt_updateUserPassword.encoding.assign(user.encoding);
	m_stmt_updateUserPassword.vip = user.vip;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_updateUserPassword.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_updateUserPassword.stmt, updateUser(user));
}

void SQLClass::removeBuddy(long userId, const std::string &uin, long buddy_id) {
	m_stmt_removeBuddy.user_id = userId;
	m_stmt_removeBuddy.uin.assign(uin);
	*m_stmt_removeBuddySettings << buddy_id;
	m_stmt_removeBuddy.stmt->execute();
	m_stmt_removeBuddySettings->execute();
}

void SQLClass::removeUser(long userId) {
	m_stmt_removeUser.userId = userId;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_removeUser.stmt->execute();
		*m_sess << "DELETE FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=?", use(m_stmt_removeUser.userId), now;
		*m_sess << "DELETE FROM " + p->configuration().sqlPrefix + "buddies_settings WHERE user_id=?", use(m_stmt_removeUser.userId), now;
		*m_sess << "DELETE FROM " + p->configuration().sqlPrefix + "users_settings WHERE user_id=?", use(m_stmt_removeUser.userId), now;
	STATEMENT_EXECUTE_END(m_stmt_removeUser.stmt, removeUser(userId));

}

void SQLClass::removeUserBuddies(long userId) {
	m_stmt_removeUserBuddies.user_id = userId;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_removeUserBuddies.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_removeUserBuddies.stmt, removeUserBuddies(userId));
}

void SQLClass::addDownload(const std::string &filename, const std::string &vip) {
}

long SQLClass::addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group, const std::string &nickname, int flags) {
	m_stmt_addBuddy.user_id = userId;
	m_stmt_addBuddy.uin.assign(uin);
	p->protocol()->prepareUsername(m_stmt_addBuddy.uin);
	m_stmt_addBuddy.subscription.assign(subscription);
	m_stmt_addBuddy.groups.assign(group);
	m_stmt_addBuddy.nickname.assign(nickname);
	m_stmt_addBuddy.flags = flags;
	try {
		m_stmt_addBuddy.stmt->execute();
	}
#ifdef WITH_SQLITE
	/* SQLite doesn't support "ON DUPLICATE UPDATE". */
	catch (Poco::Data::SQLite::ConstraintViolationException e) {
		m_stmt_updateBuddy.user_id = userId;
		m_stmt_updateBuddy.uin = uin;
		m_stmt_updateBuddy.groups = group;
		m_stmt_updateBuddy.nickname = nickname;
		m_stmt_updateBuddy.flags = flags;
		try {
			m_stmt_updateBuddy.stmt->execute();
		}
		catch (Poco::Exception e) {
			Log("SQL ERROR", e.displayText());
		}
	}
#endif
	catch (Poco::Exception e) {
		Log("SQL ERROR", e.displayText());
	}
	// It would be much more better to find out the way how to get last_inserted_rowid from Poco.
	if (p->configuration().sqlType == "sqlite") {
		Poco::UInt64 id = -1;
		STATEMENT_EXECUTE_BEGIN();
		*m_sess << "SELECT last_insert_rowid();", into(id), now;
		STATEMENT_EXECUTE_END(m_stmt_addBuddy.stmt, addBuddy(userId, uin, subscription, group, nickname));
		return id;
	}
	else
		/* This is only needed when doing an insert, so is OK even though
		 * it returns something bogus for ON DUPLICATE KEY UPPDATE.
		 */
		return Poco::AnyCast<Poco::UInt64>(m_sess->getProperty("insertId"));
}

void SQLClass::updateBuddySubscription(long userId, const std::string &uin, const std::string &subscription) {
	*m_stmt_updateBuddySubscription << subscription << userId << uin;
	
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_updateBuddySubscription->execute();
	STATEMENT_EXECUTE_END(m_stmt_updateBuddySubscription.stmt, updateBuddySubscription(userId, uin, subscription));
}

UserRow SQLClass::getUserByJid(const std::string &jid){
	UserRow user;
	user.id = -1;
	user.vip = 0;

	*m_stmt_getUserByJid << jid;
	if (m_stmt_getUserByJid->execute()) {
		*m_stmt_getUserByJid >> user.id >> user.jid >> user.uin >> user.password >> user.encoding >> user.language >> user.vip;
	}

	return user;
}

std::map<std::string, UserRow> SQLClass::getUsersByJid(const std::string &jid) {
	std::vector<Poco::Int32> resId; 
	std::vector<std::string> resJid;
	std::vector<std::string> resUin;
	std::vector<std::string> resPassword;
	std::vector<std::string> resEncoding;
	*m_sess <<  "SELECT id, jid, uin, password, encoding FROM " + p->configuration().sqlPrefix + "users WHERE jid LIKE \"" + jid +"%\"",
													into(resId),
													into(resJid),
													into(resUin),
													into(resPassword),
													into(resEncoding), now;
	std::map<std::string, UserRow> users;
	for (int i = 0; i < (int) resId.size(); i++) {
		std::string jid = resJid[i];
		users[jid].id = resId[i];
		users[jid].jid = resJid[i];
		users[jid].uin = resUin[i];
		users[jid].password = resPassword[i];
		users[jid].encoding = resEncoding[i];
	}
	return users;
}

GHashTable *SQLClass::getBuddies(long userId, PurpleAccount *account) {
	GHashTable *roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	std::vector <Poco::Int32> settingIds;
	std::vector <Poco::Int32> settingTypes;
	std::vector <std::string> settingKeys;
	std::vector <std::string> settingValues;
	
	std::vector <Poco::Int32> buddyIds;
	std::vector <Poco::Int32> buddyUserIds;
	std::vector <std::string> buddyUins;
	std::vector <std::string> buddySubscriptions;
	std::vector <std::string> buddyNicknames;
	std::vector <std::string> buddyGroups;
	std::vector <Poco::Int32> buddyFlags;

	*m_stmt_getBuddiesSettings << userId;
	m_stmt_getBuddiesSettings->execute();
	*m_stmt_getBuddiesSettings >> settingIds >> settingTypes >> settingKeys >> settingValues;

	*m_stmt_getBuddies << userId;
	m_stmt_getBuddies->execute();
	*m_stmt_getBuddies >> buddyIds >> buddyUserIds >> buddyUins >> buddySubscriptions >> buddyNicknames >> buddyGroups >> buddyFlags;

#ifndef WIN32
	double vm, rss;
	process_mem_usage(vm, rss);
	Log("MEMORY USAGE BEFORE ADDING BUDDY", rss);
#endif

	int i = 0;
	for (int k = 0; k < buddyIds.size(); k++) {
		std::string preparedUin(buddyUins[k]);
		Transport::instance()->protocol()->prepareUsername(preparedUin, account);

		// Don't add buddies with broken names (that's because of some old bugs in spectrum, we can remove it
		// if there will be some chechdb app/script)
		if (!buddyUins[k].empty() && std::count(buddyUins[k].begin(), buddyUins[k].end(), '@') <= 1 && g_hash_table_lookup(roster, preparedUin.c_str()) == NULL) {
			std::string subscription = buddySubscriptions[k].empty() ? "ask" : buddySubscriptions[k];
			std::string group = buddyGroups[k].empty() ? "Buddies" : buddyGroups[k];
			PurpleGroup *g = purple_find_group(group.c_str());
			if (!g) {
				g = purple_group_new(group.c_str());
				purple_blist_add_group(g, NULL);
			}

			PurpleBuddy *buddy = purple_find_buddy_in_group(account, buddyUins[k].c_str(), g);
			if (!buddy) {
				// create contact
				PurpleContact *contact = purple_contact_new();
				purple_blist_add_contact(contact, g, NULL);

				// create buddy
				buddy = purple_buddy_new(account, buddyUins[k].c_str(), buddyNicknames[k].c_str());
				purple_blist_add_buddy(buddy, contact, g, NULL);
				Log("ADDING BUDDY", buddyIds[k] << " " << buddyUins[k] << " subscription: " << subscription << " " << buddy);

				// add settings
				GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
				while(i < (int) settingIds.size()) {
					if (settingIds[i] == buddyIds[k]) {
						Log("ADDING SETTING ", settingIds[i] << " " << settingKeys[i]);
						PurpleType type = (PurpleType) settingTypes[i];
						PurpleValue *value = NULL;
						switch (type) {
							case PURPLE_TYPE_BOOLEAN:
								value = purple_value_new(PURPLE_TYPE_BOOLEAN);
								purple_value_set_boolean(value, atoi(settingValues[i].c_str()));
							break;
							case PURPLE_TYPE_STRING:
								value = purple_value_new(PURPLE_TYPE_STRING);
								purple_value_set_string(value, settingValues[i].c_str());
							break;
							default:
								break;
						}
						if (value)
							g_hash_table_replace(settings, g_strdup(settingKeys[i].c_str()), value);
						i++;
					}
					else
						break;
				}
				g_hash_table_destroy(buddy->node.settings);
				buddy->node.settings = settings;
			}

			SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
			if (!buddy->node.ui_data) {
				buddy->node.ui_data = (void *) new SpectrumBuddy(buddyIds[k], buddy);
				s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
				s_buddy->setSubscription(subscription);
				s_buddy->setFlags(buddyFlags[k]);
			}
			g_hash_table_replace(roster, g_strdup(preparedUin.c_str()), buddy->node.ui_data);
			
			GSList *buddies;
			for (buddies = purple_find_buddies(account, buddyUins[k].c_str()); buddies;
					buddies = g_slist_delete_link(buddies, buddies))
			{
				PurpleBuddy *buddy = (PurpleBuddy *) buddies->data;
				if (!buddy->node.ui_data) {
					buddy->node.ui_data = (void *) new SpectrumBuddy(buddyIds[k], buddy);
					SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
					s_buddy->setSubscription(subscription);
					s_buddy->setFlags(buddyFlags[k]);
				}
			}
		}
	}


#ifndef WIN32
	process_mem_usage(vm, rss);
	Log("MEMORY USAGE AFTER ADDING BUDDY", rss);
#endif
	return roster;
}

std::list <std::string> SQLClass::getBuddies(long userId) {
	std::list <std::string> list;

	std::vector <Poco::Int32> buddyIds;
	std::vector <Poco::Int32> buddyUserIds;
	std::vector <std::string> buddyUins;
	std::vector <std::string> buddySubscriptions;
	std::vector <std::string> buddyNicknames;
	std::vector <std::string> buddyGroups;
	std::vector <Poco::Int32> buddyFlags;

	*m_stmt_getBuddies << userId;
	m_stmt_getBuddies->execute();
	*m_stmt_getBuddies >> buddyIds >> buddyUserIds >> buddyUins >> buddySubscriptions >> buddyNicknames >> buddyGroups >> buddyFlags;
	
	for (int k = 0; k < buddyIds.size(); k++) {
		// TODO: move this JID escaping stuff upstream
		std::string uin = buddyUins[k];
// 		std::cout << "FLAGS " << m_stmt_getBuddies.resFlags << "\n";
		if (buddyFlags[k] & SPECTRUM_BUDDY_JID_ESCAPING) {
			list.push_back(JID::escapeNode(uin));
		}
		else {
			std::for_each( uin.begin(), uin.end(), replaceBadJidCharacters() );
			list.push_back(uin);
		}
	}

	return list;
}

// settings

void SQLClass::addSetting(long userId, const std::string &key, const std::string &value, PurpleType type) {
	if (userId == 0) {
		Log("SQL ERROR", "Trying to add user setting with user_id = 0: " << key);
		return;
	}
	*m_stmt_addSetting << userId << key << type << value;
	m_stmt_addSetting->execute();
}

void SQLClass::updateSetting(long userId, const std::string &key, const std::string &value) {
	*m_stmt_updateSetting << value << userId << key;
	m_stmt_updateSetting->execute();
}

GHashTable * SQLClass::getSettings(long userId) {
	GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
	PurpleType type;
	PurpleValue *value;

	std::vector<Poco::Int32> ids;
	std::vector<std::string> values;
	std::vector<std::string> variables;
	std::vector<Poco::Int32> types;

	*m_stmt_getSettings << userId;
	m_stmt_getSettings->execute();
	*m_stmt_getSettings >> ids >> types >> variables >> values;

	for (int i = 0; i < (int) ids.size(); i++) {
		type = (PurpleType) types[i];
		value = NULL;
		switch (type) {
			case PURPLE_TYPE_BOOLEAN:
				value = purple_value_new(PURPLE_TYPE_BOOLEAN);
				purple_value_set_boolean(value, atoi(values[i].c_str()));
			break;
			case PURPLE_TYPE_STRING:
				value = purple_value_new(PURPLE_TYPE_STRING);
				purple_value_set_string(value, values[i].c_str());
			break;
			default:
				break;
		}
		if (value)
			g_hash_table_replace(settings, g_strdup(variables[i].c_str()), value);
	}

	return settings;
}

void SQLClass::addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type) {
	*m_stmt_addBuddySetting << userId << buddyId << key << type << value;
	if (p->configuration().sqlType == "sqlite")
		*m_stmt_addBuddySetting << value;
	m_stmt_addBuddySetting->execute();

}

std::vector<std::string> SQLClass::getOnlineUsers() {
	std::vector<std::string> users;
	m_stmt_getOnlineUsers->execute();
	*m_stmt_getOnlineUsers >> users;
	return users;
}

void SQLClass::setUserOnline(long userId, bool online) {
	*m_stmt_setUserOnline << online << userId;
	m_stmt_setUserOnline->execute();
}

void SQLClass::beginTransaction() {
	m_sess->begin();
}

void SQLClass::commitTransaction() {
	m_sess->commit();
}


