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

static gboolean pingDB(gpointer data) {
	SQLClass *sql = (SQLClass *) data;
	std::cout << "PING.\n";
	sql->ping();
	return TRUE;
}

SQLClass::SQLClass(GlooxMessageHandler *parent){
	p = parent;
	m_loaded = false;

	dbi_initialize(NULL);

	m_conn = dbi_conn_new(p->configuration().sqlType.c_str());
	if (!m_conn) {
		dbi_driver driver;
		std::cout << "Libdbi driver '" << p->configuration().sqlType << "' can't be loaded.\n";
		std::cout << "Currently installed drivers are:\n";
		for (driver = dbi_driver_list(NULL); driver; driver = dbi_driver_list(driver)) {
			std::cout << "* " << dbi_driver_get_name(driver) << "\n";
		}
		return;
	}
	
	if (p->configuration().sqlType == "sqlite3") {
		dbi_conn_set_option(m_conn, "sqlite3_dbdir", g_path_get_dirname(p->configuration().sqlDb.c_str()));
		dbi_conn_set_option(m_conn, "dbname", g_path_get_basename(p->configuration().sqlDb.c_str()));
	} else {
		dbi_conn_set_option(m_conn, "host", p->configuration().sqlHost.c_str());
		dbi_conn_set_option(m_conn, "username", p->configuration().sqlUser.c_str());
		dbi_conn_set_option(m_conn, "password", p->configuration().sqlPassword.c_str());
		dbi_conn_set_option(m_conn, "dbname", p->configuration().sqlDb.c_str());
		dbi_conn_set_option(m_conn, "encoding", "UTF-8");
	}

	if (dbi_conn_connect(m_conn) < 0) {
		std::cout << "SQL CONNECTION FAILED\n";
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
	else {
		initDb();
		m_loaded = true;
		purple_timeout_add_seconds(60 * 5, &pingDB, this);
	}
	

// 	if (!vipSQL->connect("platby",p->configuration().sqlHost.c_str(),p->configuration().sqlUser.c_str(),p->configuration().sqlPassword.c_str()))
}

SQLClass::~SQLClass() {
	dbi_conn_close(m_conn);
	dbi_shutdown();
}

void SQLClass::initDb() {
	if (p->configuration().sqlType != "sqlite3")
		return;
	dbi_result result;
	int i;
	const char *create_stmts_sqlite[] = {
		"CREATE TABLE IF NOT EXISTS rosters ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"jid VARCHAR(100) NOT NULL,"
			"uin VARCHAR(100) NOT NULL,"
			"subscription VARCHAR(10) NOT NULL,"
			"nickname VARCHAR(255) NOT NULL DEFAULT \"\","
			"g VARCHAR(255) NOT NULL DEFAULT \"\","
			"UNIQUE (jid,uin)"
		");",
		"CREATE TABLE IF NOT EXISTS settings ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"jid VARCHAR(255) NOT NULL,"
			"var VARCHAR(255) NOT NULL,"
			"type INTEGER NOT NULL,"
			"value VARCHAR(255) NOT NULL"
		");",
		"CREATE TABLE IF NOT EXISTS users ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"jid VARCHAR(255) NOT NULL,"
			"uin VARCHAR(255) NOT NULL,"
			"password VARCHAR(255) NOT NULL,"
			"language VARCHAR(5) NOT NULL,"
			"`group` INTEGER NOT NULL DEFAULT 0"
		");","",""
	};
	for (i = 0; i < 5; i++) {
		result = dbi_conn_query(m_conn, create_stmts_sqlite[i]);
		if (result == NULL) {
			const char *errmsg;
			dbi_conn_error(m_conn, &errmsg);
			if (errmsg)
				Log().Get("SQL ERROR") << errmsg;
		}
		dbi_result_free(result);
	}
}

bool SQLClass::isVIP(const std::string &jid){
	dbi_result result;
	bool ret = false;

	result = dbi_conn_queryf(m_conn, "SELECT COUNT(jid) as is_vip FROM `%svips` WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str());
	if (result) {
		if (dbi_result_first_row(result)) {
			ret = true;
		}
		dbi_result_free(result);
	}
	else {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}

	return ret;
}

long SQLClass::getRegisteredUsersCount(){
	dbi_result result;
	unsigned int r = 0;

	result = dbi_conn_queryf(m_conn, "select count(*) as count from %susers", p->configuration().sqlPrefix.c_str());
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
	}

	return r;
}

long SQLClass::getRegisteredUsersRosterCount(){
	dbi_result result;
	unsigned int r = 0;

	result = dbi_conn_queryf(m_conn, "select count(*) as count from %srosters", p->configuration().sqlPrefix.c_str());
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
	}

	return r;
}

void SQLClass::updateUserPassword(const std::string &jid,const std::string &password,const std::string &language) {
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "UPDATE %susers SET password=\"%s\", language=\"%s\" WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), password.c_str(), language.c_str(), jid.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::removeUINFromRoster(const std::string &jid,const std::string &uin) {
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "DELETE FROM %srosters WHERE jid=\"%s\" AND uin=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str(), uin.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::removeUser(const std::string &jid){
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "DELETE FROM %susers WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::removeUserFromRoster(const std::string &jid){
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "DELETE FROM %srosters WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::addDownload(const std::string &filename,const std::string &vip){
}

void SQLClass::addUser(const std::string &jid,const std::string &uin,const std::string &password,const std::string &language){
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "INSERT INTO %susers (jid, uin, password, language) VALUES (\"%s\", \"%s\", \"%s\", \"%s\")", p->configuration().sqlPrefix.c_str(), jid.c_str(), uin.c_str(), password.c_str(), language.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
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

void SQLClass::addUserToRoster(const std::string &jid,const std::string &uin,const std::string &subscription, const std::string &group, const std::string &nickname) {
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "INSERT INTO %srosters (jid, uin, subscription, g, nickname) VALUES (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\") ON DUPLICATE KEY UPDATE g=\"%s\", nickname=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str(), uin.c_str(), subscription.c_str(), group.c_str(), nickname.c_str(), group.c_str(), nickname.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::updateUserRosterSubscription(const std::string &jid,const std::string &uin,const std::string &subscription){
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "UPDATE %srosters SET subscription=\"%s\" WHERE jid=\"%s\" AND uin=\"%s\"", p->configuration().sqlPrefix.c_str(), subscription.c_str(), jid.c_str(), uin.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

UserRow SQLClass::getUserByJid(const std::string &jid){
	UserRow user;
	dbi_result result;
	user.id = -1;

	result = dbi_conn_queryf(m_conn, "SELECT * FROM %susers WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str());
	if (result) {
		if (dbi_result_first_row(result)) {
			user.id = dbi_result_get_longlong(result, "id");
			user.jid = std::string(dbi_result_get_string(result, "jid"));
			user.uin = std::string(dbi_result_get_string(result, "uin"));
			user.password = std::string(dbi_result_get_string(result, "password"));
		}
		dbi_result_free(result);
	}
	else {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}

	return user;
}

std::map<std::string,RosterRow> SQLClass::getRosterByJid(const std::string &jid){
	std::map<std::string,RosterRow> rows;
	dbi_result result;

	result = dbi_conn_queryf(m_conn, "SELECT * FROM %srosters WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str());
	if (result) {
		while (dbi_result_next_row(result)) {
			RosterRow user;
			user.id = dbi_result_get_longlong(result, "id");
			user.jid = std::string(dbi_result_get_string(result, "jid"));
			user.uin = std::string(dbi_result_get_string(result, "uin"));
			user.subscription = std::string(dbi_result_get_string(result, "subscription"));
			user.nickname = std::string(dbi_result_get_string(result, "nickname"));
			user.group = std::string(dbi_result_get_string(result, "g"));
			if (user.subscription.empty())
				user.subscription="ask";
			user.online = false;
			user.lastPresence = "";
			rows[std::string(dbi_result_get_string(result, "uin"))] = user;
		}
	}
	else {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}

	return rows;
}

// settings

void SQLClass::addSetting(const std::string &jid, const std::string &key, const std::string &value, PurpleType type) {
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "INSERT INTO %ssettings (jid, var, type, value) VALUES (\"%s\",\"%s\", \"%d\", \"%s\")", p->configuration().sqlPrefix.c_str(), jid.c_str(), key.c_str(), (int) type, value.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::updateSetting(const std::string &jid, const std::string &key, const std::string &value) {
	dbi_result result;
	result = dbi_conn_queryf(m_conn, "UPDATE %ssettings SET value=\"%s\" WHERE jid=\"%s\" AND var=\"%s\"", p->configuration().sqlPrefix.c_str(), value.c_str(), jid.c_str(), key.c_str());
	if (!result) {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}
}

void SQLClass::getSetting(const std::string &jid, const std::string &key) {

}

GHashTable * SQLClass::getSettings(const std::string &jid) {
	GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
	PurpleType type;
	PurpleValue *value;

	dbi_result result;

	result = dbi_conn_queryf(m_conn, "SELECT * FROM %ssettings WHERE jid=\"%s\"", p->configuration().sqlPrefix.c_str(), jid.c_str());
	if (result) {
		while (dbi_result_next_row(result)) {
			type = (PurpleType) dbi_result_get_int(result, "type");
			if (type == PURPLE_TYPE_BOOLEAN) {
				value = purple_value_new(PURPLE_TYPE_BOOLEAN);
				purple_value_set_boolean(value, atoi(dbi_result_get_string(result, "value")));
			}
			if (type == PURPLE_TYPE_STRING) {
				value = purple_value_new(PURPLE_TYPE_STRING);
				purple_value_set_string(value, dbi_result_get_string(result, "value"));
			}
			g_hash_table_replace(settings, g_strdup(dbi_result_get_string(result, "var")), value);
		}
		dbi_result_free(result);
	}
	else {
		const char *errmsg;
		dbi_conn_error(m_conn, &errmsg);
		if (errmsg)
			Log().Get("SQL ERROR") << errmsg;
	}

	return settings;
}
