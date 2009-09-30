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

#ifndef WITH_MYSQL
#ifndef WITH_SQLITE
#ifndef WITH_ODBC
#error There is no libPocoData storage backend installed. Spectrum will not work without one of them.
#endif // WITH_ODBC
#endif // WITH_SQLITE
#endif // WITH_MYSQL

SQLClass::SQLClass(GlooxMessageHandler *parent) {
	p = parent;
	m_loaded = false;
	m_sess = NULL;
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
		Log().Get("SQL ERROR") << e.displayText();
		return;
	}
	
	if (!m_sess) {
		Log().Get("SQL ERROR") << "You don't have Spectrum compiled with the storage backend you are trying to use";
		Log().Get("SQL ERROR") << "Currently Spectrum is compiled with these storage backends:";
#ifdef WITH_MYSQL
		Log().Get("SQL ERROR") << " - MySQL - use type=mysql for this backend in config file";
#endif
#ifdef WITH_SQLITE
		Log().Get("SQL ERROR") << " - SQLite - use type=sqlite for this backend in config file";
#endif
// #ifdef WITH_ODBC
// 		Log().Get("SQL ERROR") << " - ODBC - use type=odbc for this backend in config file";
// #endif
		return;
	}
	
	// Prepared statements
	m_stmt_addUser.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "users (jid, uin, password, language) VALUES (?, ?, ?, ?)"),
										   use(m_stmt_addUser.jid),
										   use(m_stmt_addUser.uin),
										   use(m_stmt_addUser.password),
										   use(m_stmt_addUser.language) ) );
	m_stmt_updateUserPassword.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "users SET password=?, language=? WHERE jid=?)"),
													  use(m_stmt_updateUserPassword.password),
													  use(m_stmt_updateUserPassword.language),
													  use(m_stmt_updateUserPassword.jid) ) );
	m_stmt_removeBuddy.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? AND uin=?"),
											   use(m_stmt_removeBuddy.user_id),
											   use(m_stmt_removeBuddy.uin) ) );
	m_stmt_removeUser.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "users WHERE jid=?"),
											  use(m_stmt_removeUser.jid) ) );
	m_stmt_removeUserBuddies.stmt = new Statement( ( STATEMENT("DELETE FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=?"),
													 use(m_stmt_removeUserBuddies.user_id) ) );
	if (p->configuration().sqlType == "sqlite")
		m_stmt_addBuddy.stmt = new Statement( ( STATEMENT("INSERT OR REPLACE INTO " + p->configuration().sqlPrefix + "buddies (user_id, uin, subscription, groups, nickname) VALUES (?, ?, ?, ?, ?)"),
											use(m_stmt_addBuddy.user_id),
											use(m_stmt_addBuddy.uin),
											use(m_stmt_addBuddy.subscription),
											use(m_stmt_addBuddy.groups),
											use(m_stmt_addBuddy.nickname) ) );
	else
		m_stmt_addBuddy.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies (user_id, uin, subscription, groups, nickname) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE groups=?, nickname=?"),
											use(m_stmt_addBuddy.user_id),
											use(m_stmt_addBuddy.uin),
											use(m_stmt_addBuddy.subscription),
											use(m_stmt_addBuddy.groups),
											use(m_stmt_addBuddy.nickname),
											use(m_stmt_addBuddy.groups),
											use(m_stmt_addBuddy.nickname) ) );
	m_stmt_updateBuddySubscription.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "buddies SET subscription=? WHERE user_id=? AND uin=?"),
														   use(m_stmt_updateBuddySubscription.subscription),
														   use(m_stmt_updateBuddySubscription.user_id),
														   use(m_stmt_updateBuddySubscription.uin) ) );
	m_stmt_getUserByJid.stmt = new Statement( ( STATEMENT("SELECT id, jid, uin, password FROM " + p->configuration().sqlPrefix + "users WHERE jid=?"),
												use(m_stmt_getUserByJid.jid),
												into(m_stmt_getUserByJid.resId, -1),
												into(m_stmt_getUserByJid.resJid),
												into(m_stmt_getUserByJid.resUin),
												into(m_stmt_getUserByJid.resPassword),
												limit(1),
												range(0, 1) ) );
	m_stmt_getBuddies.stmt = new Statement( ( STATEMENT("SELECT id, user_id, uin, subscription, nickname, groups FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? ORDER BY id ASC"),
											  use(m_stmt_getBuddies.user_id),
											  into(m_stmt_getBuddies.resId),
											  into(m_stmt_getBuddies.resUserId),
											  into(m_stmt_getBuddies.resUin),
											  into(m_stmt_getBuddies.resSubscription),
											  into(m_stmt_getBuddies.resNickname),
											  into(m_stmt_getBuddies.resGroups),
											  range(0, 1) ) );
	m_stmt_addSetting.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "users_settings (user_id, var, type, value) VALUES (?,?,?,?)"),
											  use(m_stmt_addSetting.user_id),
											  use(m_stmt_addSetting.var),
											  use(m_stmt_addSetting.type),
											  use(m_stmt_addSetting.value) ) );
	m_stmt_updateSetting.stmt = new Statement( ( STATEMENT("UPDATE " + p->configuration().sqlPrefix + "users_settings SET value=? WHERE user_id=? AND var=?"),
												 use(m_stmt_updateSetting.value),
												 use(m_stmt_updateSetting.user_id),
												 use(m_stmt_updateSetting.var) ) );
	m_stmt_getBuddiesSettings.stmt = new Statement( ( STATEMENT("SELECT buddy_id, type, var, value FROM " + p->configuration().sqlPrefix + "buddies_settings WHERE user_id=? ORDER BY buddy_id ASC"),
													  use(m_stmt_getBuddiesSettings.user_id),
													  into(m_stmt_getBuddiesSettings.resId),
													  into(m_stmt_getBuddiesSettings.resType),
													  into(m_stmt_getBuddiesSettings.resVar),
													  into(m_stmt_getBuddiesSettings.resValue) ) );
	if (p->configuration().sqlType == "sqlite")
		m_stmt_addBuddySetting.stmt = new Statement( ( STATEMENT("INSERT OR REPLACE INTO " + p->configuration().sqlPrefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?)"),
												   use(m_stmt_addBuddySetting.user_id),
												   use(m_stmt_addBuddySetting.buddy_id),
												   use(m_stmt_addBuddySetting.var),
												   use(m_stmt_addBuddySetting.type),
												   use(m_stmt_addBuddySetting.value) ) );
	else
		m_stmt_addBuddySetting.stmt = new Statement( ( STATEMENT("INSERT INTO " + p->configuration().sqlPrefix + "buddies_settings (user_id, buddy_id, var, type, value) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE value=?"),
												   use(m_stmt_addBuddySetting.user_id),
												   use(m_stmt_addBuddySetting.buddy_id),
												   use(m_stmt_addBuddySetting.var),
												   use(m_stmt_addBuddySetting.type),
												   use(m_stmt_addBuddySetting.value),
												   use(m_stmt_addBuddySetting.value) ) );
	m_stmt_getSettings.stmt = new Statement( ( STATEMENT("SELECT user_id, type, var, value FROM " + p->configuration().sqlPrefix + "users_settings WHERE user_id=?"),
											   use(m_stmt_getSettings.user_id),
											   into(m_stmt_getSettings.resId),
											   into(m_stmt_getSettings.resType),
											   into(m_stmt_getSettings.resVar),
											   into(m_stmt_getSettings.resValue) ) );
	
	initDb();
	m_loaded = true;

// 	if (!vipSQL->connect("platby",p->configuration().sqlHost.c_str(),p->configuration().sqlUser.c_str(),p->configuration().sqlPassword.c_str()))
}

SQLClass::~SQLClass() {
	if (m_loaded) {
		m_sess->close();
		delete m_sess;
		delete m_stmt_addUser.stmt;
		delete m_stmt_updateUserPassword.stmt;
		delete m_stmt_removeBuddy.stmt;
		delete m_stmt_removeUser.stmt;
		delete m_stmt_removeUserBuddies.stmt;
		delete m_stmt_addBuddy.stmt;
		delete m_stmt_updateBuddySubscription.stmt;
		delete m_stmt_getUserByJid.stmt;
		delete m_stmt_getBuddies.stmt;
		delete m_stmt_addSetting.stmt;
		delete m_stmt_updateSetting.stmt;
		delete m_stmt_getBuddiesSettings.stmt;
		delete m_stmt_addBuddySetting.stmt;
		delete m_stmt_getSettings.stmt;
	}
}

void SQLClass::addUser(const std::string &jid,const std::string &uin,const std::string &password,const std::string &language){
	m_stmt_addUser.jid.assign(jid);
	m_stmt_addUser.uin.assign(uin);
	m_stmt_addUser.password.assign(password);
	m_stmt_addUser.language.assign(language);
	try {
		m_stmt_addUser.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

void SQLClass::initDb() {
	if (p->configuration().sqlType != "sqlite")
		return;
	try {
		*m_sess << "CREATE TABLE " + p->configuration().sqlPrefix + "buddies ("
					"  id INTEGER PRIMARY KEY NOT NULL,"
					"  user_id int(10) NOT NULL,"
					"  uin varchar(255) NOT NULL,"
					"  subscription varchar(20) NOT NULL,"
					"  nickname varchar(255) NOT NULL,"
					"  groups varchar(255) NOT NULL"
					");", now;

		*m_sess << "CREATE UNIQUE INDEX user_id ON " + p->configuration().sqlPrefix + "buddies (user_id, uin);", now;

		*m_sess << "CREATE TABLE " + p->configuration().sqlPrefix + "buddies_settings ("
					"  user_id int(10) NOT NULL,"
					"  buddy_id int(10) NOT NULL,"
					"  var varchar(50) NOT NULL,"
					"  type smallint(4) NOT NULL,"
					"  value varchar(255) NOT NULL,"
					"  PRIMARY KEY (buddy_id, var)"
					");", now;

		*m_sess << "CREATE INDEX user_id02 ON " + p->configuration().sqlPrefix + "buddies_settings (user_id);", now;

		*m_sess << "CREATE TABLE " + p->configuration().sqlPrefix + "users ("
					"  id INTEGER PRIMARY KEY NOT NULL,"
					"  jid varchar(255) NOT NULL,"
					"  uin varchar(4095) NOT NULL,"
					"  password varchar(255) NOT NULL,"
					"  language varchar(25) NOT NULL,"
					"  encoding varchar(50) NOT NULL DEFAULT 'utf8',"
					"  last_login datetime,"
					"  vip tinyint(1) NOT NULL DEFAULT '0'"
					");", now;

		*m_sess << "CREATE UNIQUE INDEX jid ON " + p->configuration().sqlPrefix + "users (jid);", now;

		*m_sess << "CREATE TABLE " + p->configuration().sqlPrefix + "users_settings ("
					"  user_id int(10) NOT NULL,"
					"  var varchar(50) NOT NULL,"
					"  type smallint(4) NOT NULL,"
					"  value varchar(255) NOT NULL,"
					"  PRIMARY KEY (user_id, var)"
					");", now;
					
		*m_sess << "CREATE INDEX user_id03 ON " + p->configuration().sqlPrefix + "users_settings (user_id);", now;
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
		return;
	}
}

bool SQLClass::isVIP(const std::string &jid) {
	return false;
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

void SQLClass::updateUserPassword(const std::string &jid,const std::string &password,const std::string &language) {
	m_stmt_updateUserPassword.jid.assign(jid);
	m_stmt_updateUserPassword.password.assign(password);
	m_stmt_updateUserPassword.language.assign(language);
	try {
		m_stmt_updateUserPassword.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

void SQLClass::removeBuddy(long userId, const std::string &uin) {
	m_stmt_removeBuddy.user_id = userId;
	m_stmt_removeBuddy.uin.assign(uin);
	try {
		m_stmt_removeBuddy.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

void SQLClass::removeUser(const std::string &jid) {
	m_stmt_removeUser.jid.assign(jid);
	try {
		m_stmt_removeUser.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

void SQLClass::removeUserBuddies(long userId) {
	m_stmt_removeUserBuddies.user_id = userId;
	try {
		m_stmt_removeUserBuddies.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

void SQLClass::addDownload(const std::string &filename, const std::string &vip) {
}



// TODO: We have to rewrite it or remove it when we find out how to do addUserToRoster for sqlite3
// void SQLClass::updateUserToRoster(const std::string &jid,const std::string &uin,const std::string &subscription, const std::string &group, const std::string &nickname) {
// 	dbi_result result;
// 	// result = dbi_conn_queryf(m_conn, "INSERT INTO %srosters (jid, uin, subscription, g, nickname) VALUES (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\") ON DUPLICATE KEY UPDATE g=\"%s\", nickname=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str(), uin.c_str(), subscription.c_str(), group.c_str(), nickname.c_str(), group.c_str(), nickname.c_str());
// 	result = dbi_conn_queryf(m_conn, "INSERT INTO %srosters (jid, uin, subscription, g, nickname) VALUES (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", p->configuration().sqlPrefix.c_str(), jid.c_str(), uin.c_str(), subscription.c_str(), group.c_str(), nickname.c_str());
// 	if (!result) {
// 		const char *errmsg;
// 		dbi_conn_error(m_conn, &errmsg);
// 		if (errmsg)
// 			Log().Get("SQL ERROR") << errmsg;
// 	}
// }

long SQLClass::addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group, const std::string &nickname) {
	m_stmt_addBuddy.user_id = userId;
	m_stmt_addBuddy.uin.assign(uin);
	m_stmt_addBuddy.subscription.assign(subscription);
	m_stmt_addBuddy.groups.assign(group);
	m_stmt_addBuddy.nickname.assign(nickname);
	try {
		m_stmt_addBuddy.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
	// It would be much more better to find out the way how to get last_inserted_rowid from Poco.
	if (p->configuration().sqlType == "sqlite") {
		Poco::UInt64 id;
		*m_sess << "SELECT id FROM " + p->configuration().sqlPrefix + "buddies WHERE user_id=? AND uin=?", use(m_stmt_addBuddy.user_id), use(m_stmt_addBuddy.uin), into(id), now;
		return id;
	}
	else
		return Poco::AnyCast<Poco::UInt64>(m_sess->getProperty("insertId"));
}

void SQLClass::updateBuddySubscription(long userId, const std::string &uin, const std::string &subscription) {
	m_stmt_updateBuddySubscription.user_id = userId;
	m_stmt_updateBuddySubscription.uin.assign(uin);
	m_stmt_updateBuddySubscription.subscription.assign(subscription);
	try {
		m_stmt_updateBuddySubscription.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

UserRow SQLClass::getUserByJid(const std::string &jid){
	UserRow user;
	user.id = -1;
	m_stmt_getUserByJid.jid.assign(jid);
	try {
		if (m_stmt_getUserByJid.stmt->execute()) {
			do {
				user.id = m_stmt_getUserByJid.resId;
				user.jid = m_stmt_getUserByJid.resJid;
				user.uin = m_stmt_getUserByJid.resUin;
				user.password = m_stmt_getUserByJid.resPassword;
				m_stmt_getUserByJid.stmt->execute();
			} while (!m_stmt_getUserByJid.stmt->done());
		}
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
	return user;
}

std::map<std::string,RosterRow> SQLClass::getBuddies(long userId, PurpleAccount *account){
	std::map<std::string,RosterRow> rows;
	m_stmt_getBuddies.user_id = userId;
	bool buddiesLoaded = false;
	int i = 0;
	
	if (account) {
		m_stmt_getBuddiesSettings.user_id = userId;
		try {
			m_stmt_getBuddiesSettings.stmt->execute();
		}
		catch (Poco::Exception e) {
			Log().Get("SQL ERROR") << e.displayText();
		}
	}

	try {
		if (m_stmt_getBuddies.stmt->execute()) {
			do {
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

				if (!buddiesLoaded && account) {
					// create group
					std::string group = user.group.empty() ? "Buddies" : user.group;
					PurpleGroup *g = purple_find_group(group.c_str());
					if (!g) {
						g = purple_group_new(group.c_str());
						purple_blist_add_group(g, NULL);
					}

					if (!purple_find_buddy_in_group(account, user.uin.c_str(), g)) {
						// create contact
						PurpleContact *contact = purple_contact_new();
						purple_blist_add_contact(contact, g, NULL);

// 						create buddy
						PurpleBuddy *buddy = purple_buddy_new(account, user.uin.c_str(), user.nickname.c_str());
						long *id = new long(user.id);
						buddy->node.ui_data = (void *) id;
						purple_blist_add_buddy(buddy, contact, g, NULL);
						GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
						std::cout << "ADDING BUDDY " << " " << user.id << " " << user.uin << "\n";
						while(i < (int) m_stmt_getBuddiesSettings.resId.size()) {
							std::cout << m_stmt_getBuddiesSettings.resId[i] << "\n";
							if (m_stmt_getBuddiesSettings.resId[i] == user.id) {
								std::cout << "ADDING SETTING " << m_stmt_getBuddiesSettings.resVar[i] << "\n";
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
				}

				rows[std::string(m_stmt_getBuddies.resUin)] = user;
				m_stmt_getBuddies.stmt->execute();
			} while (!m_stmt_getBuddies.stmt->done());
		}
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}

	m_stmt_getBuddiesSettings.resId.clear();
	m_stmt_getBuddiesSettings.resType.clear();
	m_stmt_getBuddiesSettings.resValue.clear();
	m_stmt_getBuddiesSettings.resVar.clear();

	return rows;
}

// settings

void SQLClass::addSetting(long userId, const std::string &key, const std::string &value, PurpleType type) {
	m_stmt_addSetting.user_id = userId;
	m_stmt_addSetting.var.assign(key);
	m_stmt_addSetting.value.assign(value);
	m_stmt_addSetting.type = type;
	try {
		m_stmt_addSetting.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

void SQLClass::updateSetting(long userId, const std::string &key, const std::string &value) {
	m_stmt_updateSetting.user_id = userId;
	m_stmt_updateSetting.var.assign(key);
	m_stmt_updateSetting.value.assign(value);
	try {
		m_stmt_updateSetting.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

GHashTable * SQLClass::getSettings(long userId) {
	GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
	PurpleType type;
	PurpleValue *value;
	int i;
	m_stmt_getSettings.user_id = userId;
	try {
		m_stmt_getSettings.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}

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
	try {
		m_stmt_addBuddySetting.stmt->execute();
	}
	catch (Poco::Exception e) {
		Log().Get("SQL ERROR") << e.displayText();
	}
}

