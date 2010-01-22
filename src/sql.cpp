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

#if !defined(WITH_MYSQL) && !defined(WITH_SQLITE) && !defined(WITH_ODBC)
#error There is no libPocoData storage backend installed. Spectrum will not work without one of them.
#endif

#ifdef WITH_SQLITE
#include <Poco/Data/SQLite/SQLiteException.h>
#endif

SQLClass::SQLClass(GlooxMessageHandler *parent, bool upgrade) {
	p = parent;
	m_loaded = false;
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
	m_stmt_updateBuddySubscription.stmt = NULL;
	m_stmt_getUserByJid.stmt = NULL;
	m_stmt_getBuddies.stmt = NULL;
	m_stmt_addSetting.stmt = NULL;
	m_stmt_updateSetting.stmt = NULL;
	m_stmt_getBuddiesSettings.stmt = NULL;
	m_stmt_addBuddySetting.stmt = NULL;
	m_stmt_getSettings.stmt = NULL;
	m_stmt_getOnlineUsers.stmt = NULL;
	m_stmt_setUserOnline.stmt = NULL;
	m_stmt_removeBuddySettings.stmt = NULL;
	
	m_error = 0;
	
	
	try {
#ifdef WITH_MYSQL
		if (p->configuration().sqlType == "mysql") {
			MySQL::Connector::registerConnector(); 
			m_sess = new Session("MySQL", "user=" + p->configuration().sqlUser + ";password=" + p->configuration().sqlPassword + ";host=" + p->configuration().sqlHost + ";db=" + p->configuration().sqlDb + ";auto-reconnect=true");
		}
#endif
#ifdef WITH_SQLITE
		if (p->configuration().sqlType == "sqlite") {
			SQLite::Connector::registerConnector(); 
			m_sess = new Session("SQLite", p->configuration().sqlDb);
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
	
	createStatements();
	
	initDb();

// 	if (!vipSQL->connect("platby",p->configuration().sqlHost.c_str(),p->configuration().sqlUser.c_str(),p->configuration().sqlPassword.c_str()))
}

SQLClass::~SQLClass() {
	if (m_loaded) {
		m_sess->close();
		delete m_sess;
		removeStatements();
	}
}

void SQLClass::createStatements() {
	if (!m_version_stmt) {
		Log("NEW STATEMENT", "version_stmt");
		m_version_stmt = new Statement( ( STATEMENT("SELECT ver FROM " + p->configuration().sqlPrefix + "db_version") ) );
	}
	
	// Prepared statements
	if (!m_stmt_addUser.stmt)
		m_stmt_addUser.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "users (jid, uin, password, language, encoding) VALUES (?, ?, ?, ?, ?)"),
											use(m_stmt_addUser.jid),
											use(m_stmt_addUser.uin),
											use(m_stmt_addUser.password),
											use(m_stmt_addUser.language),
											use(m_stmt_addUser.encoding) ) );
	if (!m_stmt_updateUserPassword.stmt)
		m_stmt_updateUserPassword.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "users SET password=?, language=?, encoding=? WHERE jid=?"),
														use(m_stmt_updateUserPassword.password),
														use(m_stmt_updateUserPassword.language),
														use(m_stmt_updateUserPassword.encoding),
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
			m_stmt_addBuddy.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies (user_id, uin, subscription, groups, nickname) VALUES (?, ?, ?, ?, ?)"),
												use(m_stmt_addBuddy.user_id),
												use(m_stmt_addBuddy.uin),
												use(m_stmt_addBuddy.subscription),
												use(m_stmt_addBuddy.groups),
												use(m_stmt_addBuddy.nickname) ) );
		if (!m_stmt_updateBuddy.stmt)
			m_stmt_updateBuddy.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "buddies SET groups=?, nickname=? WHERE user_id=? AND uin=?"),
												use(m_stmt_updateBuddy.groups),
												use(m_stmt_updateBuddy.nickname),
												use(m_stmt_updateBuddy.user_id),
												use(m_stmt_updateBuddy.uin) ) );
	} else
#endif
		if (!m_stmt_addBuddy.stmt)
			m_stmt_addBuddy.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies (user_id, uin, subscription, groups, nickname) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE groups=?, nickname=?"),
												use(m_stmt_addBuddy.user_id),
												use(m_stmt_addBuddy.uin),
												use(m_stmt_addBuddy.subscription),
												use(m_stmt_addBuddy.groups),
												use(m_stmt_addBuddy.nickname),
												use(m_stmt_addBuddy.groups),
												use(m_stmt_addBuddy.nickname) ) );
	if (!m_stmt_updateBuddySubscription.stmt)
		m_stmt_updateBuddySubscription.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "buddies SET subscription=? WHERE user_id=? AND uin=?"),
															use(m_stmt_updateBuddySubscription.subscription),
															use(m_stmt_updateBuddySubscription.user_id),
															use(m_stmt_updateBuddySubscription.uin) ) );
	if (!m_stmt_getUserByJid.stmt)
		m_stmt_getUserByJid.stmt = new Statement( ( STATEMENT("SELECT id, jid, uin, password, encoding FROM " + p->configuration().sqlPrefix + "users WHERE jid=?"),
													use(m_stmt_getUserByJid.jid),
													into(m_stmt_getUserByJid.resId, -1),
													into(m_stmt_getUserByJid.resJid),
													into(m_stmt_getUserByJid.resUin),
													into(m_stmt_getUserByJid.resPassword),
													into(m_stmt_getUserByJid.resEncoding),
													limit(1),
													range(0, 1) ) );
	if (!m_stmt_getBuddies.stmt)
		m_stmt_getBuddies.stmt = new Statement( ( STATEMENT("SELECT id, user_id, uin, subscription, nickname, groups FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? ORDER BY id ASC"),
												use(m_stmt_getBuddies.user_id),
												into(m_stmt_getBuddies.resId),
												into(m_stmt_getBuddies.resUserId),
												into(m_stmt_getBuddies.resUin),
												into(m_stmt_getBuddies.resSubscription),
												into(m_stmt_getBuddies.resNickname),
												into(m_stmt_getBuddies.resGroups),
												range(0, 1) ) );
	if (!m_stmt_addSetting.stmt)
		m_stmt_addSetting.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)"),
												use(m_stmt_addSetting.user_id),
												use(m_stmt_addSetting.var),
												use(m_stmt_addSetting.type),
												use(m_stmt_addSetting.value) ) );
	if (!m_stmt_updateSetting.stmt)
		m_stmt_updateSetting.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "users_settings SET value=? WHERE user_id=? AND var=?"),
													use(m_stmt_updateSetting.value),
													use(m_stmt_updateSetting.user_id),
													use(m_stmt_updateSetting.var) ) );
	if (!m_stmt_getBuddiesSettings.stmt)
		m_stmt_getBuddiesSettings.stmt = new Statement( ( STATEMENT("SELECT buddy_id, type, var, value FROM " + p->configuration().sqlPrefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC"),
														use(m_stmt_getBuddiesSettings.user_id),
														into(m_stmt_getBuddiesSettings.resId),
														into(m_stmt_getBuddiesSettings.resType),
														into(m_stmt_getBuddiesSettings.resVar),
														into(m_stmt_getBuddiesSettings.resValue) ) );
#ifdef WITH_SQLITE
	if (p->configuration().sqlType == "sqlite") {
		if (!m_stmt_addBuddySetting.stmt)
			m_stmt_addBuddySetting.stmt = new Statement( ( STATEMENT("INSERT OR REPLACE INTO " + p->configuration().sqlPrefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)"),
													use(m_stmt_addBuddySetting.user_id),
													use(m_stmt_addBuddySetting.buddy_id),
													use(m_stmt_addBuddySetting.var),
													use(m_stmt_addBuddySetting.type),
													use(m_stmt_addBuddySetting.value) ) );
	}
	else
#endif
		if (!m_stmt_addBuddySetting.stmt)
			m_stmt_addBuddySetting.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE value=?"),
													use(m_stmt_addBuddySetting.user_id),
													use(m_stmt_addBuddySetting.buddy_id),
													use(m_stmt_addBuddySetting.var),
													use(m_stmt_addBuddySetting.type),
													use(m_stmt_addBuddySetting.value),
													use(m_stmt_addBuddySetting.value) ) );
	if (!m_stmt_removeBuddySettings.stmt)
		m_stmt_removeBuddySettings.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "buddies_settings WHERE buddy_id=?"),
												use(m_stmt_removeBuddySettings.buddy_id) ) );
	if (!m_stmt_getSettings.stmt)
		m_stmt_getSettings.stmt = new Statement( ( STATEMENT("SELECT user_id, type, var, value FROM " + p->configuration().sqlPrefix + "users_settings WHERE user_id=?"),
												use(m_stmt_getSettings.user_id),
												into(m_stmt_getSettings.resId),
												into(m_stmt_getSettings.resType),
												into(m_stmt_getSettings.resVar),
												into(m_stmt_getSettings.resValue) ) );
	if (!m_stmt_getOnlineUsers.stmt)
		m_stmt_getOnlineUsers.stmt = new Statement( ( STATEMENT("SELECT jid FROM " + p->configuration().sqlPrefix + "users WHERE online=1"),
													into(m_stmt_getOnlineUsers.resUsers) ) );
	if (!m_stmt_setUserOnline.stmt)
		m_stmt_setUserOnline.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "users SET online=? WHERE id=?"),
														use(m_stmt_setUserOnline.online),
														use(m_stmt_setUserOnline.user_id) ) );
}

void SQLClass::addUser(const std::string &jid,const std::string &uin,const std::string &password,const std::string &language, const std::string &encoding){
	m_stmt_addUser.jid.assign(jid);
	m_stmt_addUser.uin.assign(uin);
	m_stmt_addUser.password.assign(password);
	m_stmt_addUser.language.assign(language);
	m_stmt_addUser.encoding.assign(encoding);
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
	delete m_stmt_removeBuddySettings.stmt;
#ifdef WITH_SQLITE
	delete m_stmt_updateBuddy.stmt;
#endif
	delete m_stmt_updateBuddySubscription.stmt;
	delete m_stmt_getBuddies.stmt;
	delete m_stmt_addSetting.stmt;
	delete m_stmt_updateSetting.stmt;
	delete m_stmt_getUserByJid.stmt;
	delete m_stmt_getBuddiesSettings.stmt;
	delete m_stmt_addBuddySetting.stmt;
	delete m_stmt_getSettings.stmt;
	delete m_stmt_getOnlineUsers.stmt;
	delete m_stmt_setUserOnline.stmt;
	
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
	m_stmt_getUserByJid.stmt = NULL;
	m_stmt_getBuddies.stmt = NULL;
	m_stmt_addSetting.stmt = NULL;
	m_stmt_updateSetting.stmt = NULL;
	m_stmt_getBuddiesSettings.stmt = NULL;
	m_stmt_addBuddySetting.stmt = NULL;
	m_stmt_getSettings.stmt = NULL;
	m_stmt_getOnlineUsers.stmt = NULL;
	m_stmt_setUserOnline.stmt = NULL;
	m_stmt_removeBuddySettings.stmt = NULL;
}

void SQLClass::reconnect() {
	removeStatements();
	m_sess->close();
	delete m_sess;
	m_sess = new Session("MySQL", "user=" + p->configuration().sqlUser + ";password=" + p->configuration().sqlPassword + ";host=" + p->configuration().sqlHost + ";db=" + p->configuration().sqlDb + ";auto-reconnect=true");
	createStatements();
}

void SQLClass::initDb() {
	if (p->configuration().sqlType == "sqlite") {
		try {
			*m_sess << "CREATE TABLE IF NOT EXISTS " + p->configuration().sqlPrefix + "buddies ("
						"  id INTEGER PRIMARY KEY NOT NULL,"
						"  user_id int(10) NOT NULL,"
						"  uin varchar(255) NOT NULL,"
						"  subscription varchar(20) NOT NULL,"
						"  nickname varchar(255) NOT NULL,"
						"  groups varchar(255) NOT NULL"
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
						"  vip tinyint(1) NOT NULL DEFAULT '0',"
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
				"  ver INTEGER NOT NULL DEFAULT '1'"
				");", now;
			
			*m_sess << "INSERT INTO " + p->configuration().sqlPrefix + "db_version ('ver') VALUES (1);", now;
		}
		catch (Poco::Exception e) {
			Log("SQL ERROR", e.displayText());
		}
	}

	try {
		m_version_stmt->execute();
	}
	catch (Poco::Exception e) {
		m_version = 0;
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
	Log("SQL", "Current DB version: " << m_version);
	if ((p->configuration().sqlType == "sqlite" || (p->configuration().sqlType != "sqlite" && m_upgrade)) && m_version < DB_VERSION) {
		Log("SQL", "Starting DB upgrade.");
		upgradeDatabase();
		return;
	}
	
	m_loaded = true;

}

void SQLClass::upgradeDatabase() {
	try {
		for (int i = (int) m_version; i < DB_VERSION; i++) {
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
				*m_sess << "INSERT INTO " + p->configuration().sqlPrefix + "db_version (ver) VALUES ('1');", now;
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

bool SQLClass::isVIP(const std::string &jid) {
	return true;
}

long SQLClass::getRegisteredUsersCount(){
// 	dbi_result result;
	unsigned int r = 0;

// 	result = dbi_conn_queryf(m_conn, "select count(*) as count from %susers", p->configuration().sqlPrefix.c_str());
// 	if (result) {
// 		if (dbi_result_first_row(result)) {
// 			r = dbi_result_get_uint(result, "count");
// 		}
// 		dbi_result_free(result);
// 	}
// 	else {
// 		const char *errmsg;
// 		dbi_conn_error(m_conn, &errmsg);
// 		if (errmsg)
// 			Log().Get("SQL ERROR") << errmsg;
// 	}

	return r;
}

long SQLClass::getRegisteredUsersRosterCount(){
// 	dbi_result result;
	unsigned int r = 0;
/*
	result = dbi_conn_queryf(m_conn, "select count(*) as count from %sbuddies", p->configuration().sqlPrefix.c_str());
	if (result) {
		if (dbi_result_first_row(result)) {
			r = dbi_result_get_uint(result, "count");
		}
		dbi_result_free(result);
	}
	else {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}*/

	return r;
}

void SQLClass::updateUser(const UserRow &user) {
	m_stmt_updateUserPassword.jid.assign(user.jid);
	m_stmt_updateUserPassword.password.assign(user.password);
	m_stmt_updateUserPassword.language.assign(user.language);
	m_stmt_updateUserPassword.encoding.assign(user.encoding);
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_updateUserPassword.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_updateUserPassword.stmt, updateUser(user));
}

void SQLClass::removeBuddy(long userId, const std::string &uin, long buddy_id) {
	m_stmt_removeBuddy.user_id = userId;
	m_stmt_removeBuddy.uin.assign(uin);
	m_stmt_removeBuddySettings.buddy_id = buddy_id;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_removeBuddy.stmt->execute();
		m_stmt_removeBuddySettings.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_removeBuddy.stmt, removeBuddy(userId, uin, buddy_id));
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

long SQLClass::addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group, const std::string &nickname) {
	m_stmt_addBuddy.user_id = userId;
	m_stmt_addBuddy.uin.assign(uin);
	m_stmt_addBuddy.subscription.assign(subscription);
	m_stmt_addBuddy.groups.assign(group);
	m_stmt_addBuddy.nickname.assign(nickname);
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
		Log("addBuddy","");
		Log("addBuddy","Trying to get " << m_stmt_addBuddy.user_id << " " << m_stmt_addBuddy.uin);
		STATEMENT_EXECUTE_BEGIN();
		*m_sess << "SELECT id FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? AND uin=?", use(m_stmt_addBuddy.user_id), use(m_stmt_addBuddy.uin), into(id), now;
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
	m_stmt_updateBuddySubscription.user_id = userId;
	m_stmt_updateBuddySubscription.uin.assign(uin);
	m_stmt_updateBuddySubscription.subscription.assign(subscription);
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_updateBuddySubscription.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_updateBuddySubscription.stmt, updateBuddySubscription(userId, uin, subscription));
}

UserRow SQLClass::getUserByJid(const std::string &jid){
	UserRow user;
	user.id = -1;
	m_stmt_getUserByJid.jid.assign(jid);

	STATEMENT_EXECUTE_BEGIN();
		if (m_stmt_getUserByJid.stmt->execute()) {
			do {
				user.id = m_stmt_getUserByJid.resId;
				user.jid = m_stmt_getUserByJid.resJid;
				user.uin = m_stmt_getUserByJid.resUin;
				user.password = m_stmt_getUserByJid.resPassword;
				user.encoding = m_stmt_getUserByJid.resEncoding;
				m_stmt_getUserByJid.stmt->execute();
			} while (!m_stmt_getUserByJid.stmt->done());
		}
	STATEMENT_EXECUTE_END(m_stmt_getUserByJid.stmt, getUserByJid(jid));

	return user;
}

GHashTable *SQLClass::getBuddies(long userId, PurpleAccount *account){
	GHashTable *roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	m_stmt_getBuddies.user_id = userId;
	bool buddiesLoaded = false;
	int i = 0;
	
	m_stmt_getBuddiesSettings.user_id = userId;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_getBuddiesSettings.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_getBuddiesSettings.stmt, getBuddies(userId, account));

	STATEMENT_EXECUTE_BEGIN();
		do {
			if (!m_stmt_getBuddies.stmt->execute())
				break;
			// TODO: REMOVE ME AND REPlACE ALL MY OCCURS IN THIS FUNCTION
			RosterRow user;
			user.id = m_stmt_getBuddies.resId;
// 			user.jid = m_stmt_getBuddies.resJid;
			user.uin = m_stmt_getBuddies.resUin;
			user.subscription = m_stmt_getBuddies.resSubscription;
			user.nickname = m_stmt_getBuddies.resNickname;
			user.group = m_stmt_getBuddies.resGroups;
			if (user.subscription.empty())
				user.subscription = "ask";
			user.online = false;
			user.lastPresence = "";

// 			if (!buddiesLoaded) {
				// create group
				std::string group = user.group.empty() ? "Buddies" : user.group;
				PurpleGroup *g = purple_find_group(group.c_str());
				if (!g) {
					g = purple_group_new(group.c_str());
					purple_blist_add_group(g, NULL);
				}
				PurpleBuddy *buddy = purple_find_buddy_in_group(account, user.uin.c_str(), g);
				if (!buddy) {
					// create contact
					PurpleContact *contact = purple_contact_new();
					purple_blist_add_contact(contact, g, NULL);

// 						create buddy
					buddy = purple_buddy_new(account, user.uin.c_str(), user.nickname.c_str());
					buddy->node.ui_data = (void *) new SpectrumBuddy(user.id, buddy);
					purple_blist_add_buddy(buddy, contact, g, NULL);
					GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
					Log("ADDING BUDDY ", " " << user.id << " " << user.uin << " " << buddy << " " << buddy->node.ui_data);
					while(i < (int) m_stmt_getBuddiesSettings.resId.size()) {
						if (m_stmt_getBuddiesSettings.resId[i] == user.id) {
							Log("ADDING SETTING ", m_stmt_getBuddiesSettings.resVar[i]);
							PurpleType type = (PurpleType) m_stmt_getBuddiesSettings.resType[i];
							PurpleValue *value;
							if (type == PURPLE_TYPE_BOOLEAN) {
								value = purple_value_new(PURPLE_TYPE_BOOLEAN);
								purple_value_set_boolean(value, atoi(m_stmt_getBuddiesSettings.resValue[i].c_str()));
							}
							if (type == PURPLE_TYPE_STRING) {
								value = purple_value_new(PURPLE_TYPE_STRING);
								purple_value_set_string(value, m_stmt_getBuddiesSettings.resValue[i].c_str());
							}
							g_hash_table_replace(settings, g_strdup(m_stmt_getBuddiesSettings.resVar[i].c_str()), value);
							i++;
						}
						else
							break;
					}
					// set settings
					g_hash_table_destroy(buddy->node.settings);
					buddy->node.settings = settings;
				}
				else
					buddiesLoaded = true;
// 			if (!buddy->node.ui_data)
// 				buddy->node.ui_data = (void *) new SpectrumBuddy(user.id, buddy);
// 			}

// 			rows[std::string(m_stmt_getBuddies.resUin)] = user;
			g_hash_table_replace(roster, g_strdup(m_stmt_getBuddies.resUin.c_str()), buddy->node.ui_data);
// 			m_stmt_getBuddies.stmt->execute();
		} while (!m_stmt_getBuddies.stmt->done());
	STATEMENT_EXECUTE_END(m_stmt_getBuddies.stmt, getBuddies(userId, account));

	m_stmt_getBuddiesSettings.resId.clear();
	m_stmt_getBuddiesSettings.resType.clear();
	m_stmt_getBuddiesSettings.resValue.clear();
	m_stmt_getBuddiesSettings.resVar.clear();

	return roster;
}

// settings

void SQLClass::addSetting(long userId, const std::string &key, const std::string &value, PurpleType type) {
	if (userId == 0) {
		Log("SQL ERROR", "Trying to add user setting with user_id = 0: " << key);
		return;
	}
	m_stmt_addSetting.user_id = userId;
	m_stmt_addSetting.var.assign(key);
	m_stmt_addSetting.value.assign(value);
	m_stmt_addSetting.type = type;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_addSetting.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_addSetting.stmt, addSetting(userId, key, value, type));
}

void SQLClass::updateSetting(long userId, const std::string &key, const std::string &value) {
	m_stmt_updateSetting.user_id = userId;
	m_stmt_updateSetting.var.assign(key);
	m_stmt_updateSetting.value.assign(value);
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_updateSetting.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_updateSetting.stmt, updateSetting(userId, key, value));
}

GHashTable * SQLClass::getSettings(long userId) {
	GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
	PurpleType type;
	PurpleValue *value;
	int i;
	m_stmt_getSettings.user_id = userId;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_getSettings.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_getSettings.stmt, getSettings(userId));

	for (i = 0; i < (int) m_stmt_getSettings.resId.size(); i++) {
		type = (PurpleType) m_stmt_getSettings.resType[i];
		if (type == PURPLE_TYPE_BOOLEAN) {
			value = purple_value_new(PURPLE_TYPE_BOOLEAN);
			purple_value_set_boolean(value, atoi(m_stmt_getSettings.resValue[i].c_str()));
		}
		if (type == PURPLE_TYPE_STRING) {
			value = purple_value_new(PURPLE_TYPE_STRING);
			purple_value_set_string(value, m_stmt_getSettings.resValue[i].c_str());
		}
		g_hash_table_replace(settings, g_strdup(m_stmt_getSettings.resVar[i].c_str()), value);
	}

	m_stmt_getSettings.resId.clear();
	m_stmt_getSettings.resType.clear();
	m_stmt_getSettings.resValue.clear();
	m_stmt_getSettings.resVar.clear();

	return settings;
}

void SQLClass::addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type) {
	m_stmt_addBuddySetting.user_id = userId;
	m_stmt_addBuddySetting.buddy_id = buddyId;
	m_stmt_addBuddySetting.var.assign(key);
	m_stmt_addBuddySetting.value.assign(value);
	m_stmt_addBuddySetting.type = (int) type;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_addBuddySetting.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_addBuddySetting.stmt, addBuddySetting(userId, buddyId, key, value, type));
}

std::vector<std::string> SQLClass::getOnlineUsers() {
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_getOnlineUsers.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_getOnlineUsers.stmt, getOnlineUsers());
	return m_stmt_getOnlineUsers.resUsers;
}

void SQLClass::setUserOnline(long userId, bool online) {
	m_stmt_setUserOnline.user_id = userId;
	m_stmt_setUserOnline.online = online;
	STATEMENT_EXECUTE_BEGIN();
		m_stmt_setUserOnline.stmt->execute();
	STATEMENT_EXECUTE_END(m_stmt_setUserOnline.stmt, setUserOnline(userId, online));
}

