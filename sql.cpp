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

SQLClass::SQLClass(GlooxMessageHandler *parent){
	p=parent;
	sql = new mysqlpp::Connection(false);
	if (!sql->connect(p->configuration().sqlDb.c_str(),p->configuration().sqlHost.c_str(),p->configuration().sqlUser.c_str(),p->configuration().sqlPassword.c_str()))
		std::cout << "SQL CONNECTION FAILED\n";
	vipSQL = new mysqlpp::Connection(false);
	if (!vipSQL->connect("platby",p->configuration().sqlHost.c_str(),p->configuration().sqlUser.c_str(),p->configuration().sqlPassword.c_str()))
		std::cout << "SQL CONNECTION to VIP database FAILED\n";
}

bool SQLClass::isVIP(const std::string &jid){
	if (!vipSQL->connected())
		return true;
	mysqlpp::Query query = vipSQL->query();
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res;
#else
	mysqlpp::StoreQueryResult res;
#endif
	mysqlpp::Row myrow;
	query << "SELECT COUNT(jid) as is_vip FROM `users` WHERE jid='"<< jid <<"' and expire>NOW();";
	res = query.store();
#if MYSQLPP_HEADER_VERSION < 0x030000
	myrow = res.fetch_row();
	if (int(myrow.at(0))==0)
		return false;
	else
		return true;
#else
	mysqlpp::StoreQueryResult::size_type i;
	for (i = 0; i < res.num_rows(); ++i) {
		myrow = res[i];
		if (int(myrow.at(0))==0)
			return false;
		else
			return true;
	}
	return true;
#endif
}

long SQLClass::getRegisteredUsersCount(){
	mysqlpp::Query query = sql->query();
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res;
#else
	mysqlpp::StoreQueryResult res;
#endif
	mysqlpp::Row myrow;
	query << "select count(*) as count from "<< p->configuration().sqlPrefix <<"users";
	res = query.store();
#if MYSQLPP_HEADER_VERSION < 0x030000
	if (res){
		myrow = res.fetch_row();
		return long(myrow.at(0));
	}
	else return 0;
#else
	mysqlpp::StoreQueryResult::size_type i;
	for (i = 0; i < res.num_rows(); ++i) {
		myrow = res[i];
		return long(myrow.at(0));
	}
	return 0;
#endif
}

long SQLClass::getRegisteredUsersRosterCount(){
	mysqlpp::Query query = sql->query();
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res;
#else
	mysqlpp::StoreQueryResult res;
#endif
	mysqlpp::Row myrow;
	query << "select count(*) as count from "<< p->configuration().sqlPrefix <<"rosters";
	res = query.store();
#if MYSQLPP_HEADER_VERSION < 0x030000
	if (res){
		myrow = res.fetch_row();
		return long(myrow.at(0));
	}
	else return 0;
#else
	mysqlpp::StoreQueryResult::size_type i;
	for (i = 0; i < res.num_rows(); ++i) {
		myrow = res[i];
		return long(myrow.at(0));
	}
	return 0;
#endif
	
}

void SQLClass::updateUserPassword(const std::string &jid,const std::string &password,const std::string &language) {
	mysqlpp::Query query = sql->query();
	query << "UPDATE "<< p->configuration().sqlPrefix <<"users SET password=\"" << password <<"\", language=\""<< language <<"\" WHERE jid=\"" << jid << "\";";
	query.execute();
}

void SQLClass::removeUINFromRoster(const std::string &jid,const std::string &uin) {
	mysqlpp::Query query = sql->query();
	query << "DELETE FROM "<< p->configuration().sqlPrefix <<"rosters WHERE jid=\"" << jid << "\" AND uin=\"" << uin << "\";";
	query.execute();
}

void SQLClass::removeUser(const std::string &jid){
	mysqlpp::Query query = sql->query();
	query << "DELETE FROM "<< p->configuration().sqlPrefix <<"users WHERE jid=\"" << jid << "\" ;";
	query.execute();
}

void SQLClass::removeUserFromRoster(const std::string &jid){
	mysqlpp::Query query = sql->query();
	query << "DELETE FROM "<< p->configuration().sqlPrefix <<"rosters WHERE jid=\"" << jid << "\" ;";
	query.execute();
}

void SQLClass::addDownload(const std::string &filename,const std::string &vip){
	mysqlpp::Query query = sql->query();
	query << "INSERT INTO downloads " << "(filename,vip) VALUES (\"" << filename << "\"," << vip << ")";
	query.execute();
}

void SQLClass::addUser(const std::string &jid,const std::string &uin,const std::string &password,const std::string &language){
	mysqlpp::Query query = sql->query();
	query << "INSERT INTO "<< p->configuration().sqlPrefix <<"users " << "(id, jid, uin, password, language) VALUES (\"\",\"" << jid << "\",\"" << uin << "\", \"" << password << "\", \"" << language << "\")";
	query.execute();
}

void SQLClass::addUserToRoster(const std::string &jid,const std::string &uin,const std::string subscription){
	mysqlpp::Query query = sql->query();
	query << "INSERT INTO "<< p->configuration().sqlPrefix <<"rosters " << "(id, jid, uin, subscription) VALUES (\"\",\"" << jid << "\",\"" << uin << "\", \"" << subscription << "\")";
	query.execute();
	std::cout << "query2!!!\n";
}

void SQLClass::updateUserRosterSubscription(const std::string &jid,const std::string &uin,const std::string subscription){
	mysqlpp::Query query = sql->query();
	query << "UPDATE "<< p->configuration().sqlPrefix <<"rosters SET subscription=\"" << subscription << "\" WHERE jid=\"" << jid << "\" AND uin=\"" << uin << "\";";
	query.execute();
	std::cout << "query!!!\n";
}

UserRow SQLClass::getUserByJid(const std::string &jid){
	UserRow user;
	mysqlpp::Query query = sql->query();
	query << "SELECT * FROM "<< p->configuration().sqlPrefix <<"users WHERE jid=\"" << jid << "\";";
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res = query.store();
#else
	mysqlpp::StoreQueryResult res = query.store();
#endif
	user.id=-1;
	user.jid="";
	user.uin="";
	user.password="";
#if MYSQLPP_HEADER_VERSION < 0x030000
	if (res.size()) {
		mysqlpp::Row row = res.fetch_row();
		user.id=(long) row["id"];
		user.jid=(std::string)row["jid"];
		user.uin=(std::string)row["uin"];
		user.password=(std::string)row["password"];
		user.category=(int) row["group"];
	}
#else
	mysqlpp::StoreQueryResult::size_type i;
	mysqlpp::Row row;
	if (res.num_rows() > 0) {
		row = res[0];
		user.id=(long) row["id"];
		user.jid=(std::string)row["jid"];
		user.uin=(std::string)row["uin"];
		user.password=(std::string)row["password"];
		user.category=(int) row["group"];
	}
#endif
	return user;
}

std::map<std::string,RosterRow> SQLClass::getRosterByJid(const std::string &jid){
	std::map<std::string,RosterRow> rows;
	mysqlpp::Query query = sql->query();
	query << "SELECT * FROM "<< p->configuration().sqlPrefix <<"rosters WHERE jid=\"" << jid << "\";";
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res = query.store();
#else
	mysqlpp::StoreQueryResult res = query.store();
#endif

#if MYSQLPP_HEADER_VERSION < 0x030000
	if (res) {
		mysqlpp::Row row;
		while(row = res.fetch_row()){
			RosterRow user;
			user.id=(long) row["id"];
			user.jid=(std::string)row["jid"];
			user.uin=(std::string)row["uin"];
			user.subscription=(std::string)row["subscription"];
			user.nickname=(std::string)row["nickname"];
			user.group=(std::string)row["group"];
			if (user.subscription.empty())
				user.subscription="ask";
			user.online=false;
			user.lastPresence="";
			rows[(std::string)row["uin"]]=user;
		}
	}
#else
	mysqlpp::StoreQueryResult::size_type i;
	mysqlpp::Row row;
	for (i = 0; i < res.num_rows(); ++i) {
		row = res[i];
		RosterRow user;
		user.id=(long) row["id"];
		user.jid=(std::string)row["jid"];
		user.uin=(std::string)row["uin"];
		user.subscription=(std::string)row["subscription"];
		user.nickname=(std::string)row["nickname"];
		user.group=(std::string)row["group"];
		if (user.subscription.empty())
			user.subscription="ask";
		user.online=false;
		user.lastPresence="";
		rows[(std::string)row["uin"]]=user;
	}
#endif
	return rows;
}

std::map<std::string,RosterRow> SQLClass::getRosterByJidAsk(const std::string &jid){
	std::map<std::string,RosterRow> rows;
	mysqlpp::Query query = sql->query();
	query << "SELECT * FROM "<< p->configuration().sqlPrefix <<"rosters WHERE jid=\"" << jid << "\" AND subscription=\"ask\";";
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res = query.store();
#else
	mysqlpp::StoreQueryResult res = query.store();
#endif
#if MYSQLPP_HEADER_VERSION < 0x030000
	if (res) {
		mysqlpp::Row row;
		while(row = res.fetch_row()){
			RosterRow user;
			user.id=(long) row["id"];
			user.jid=(std::string)row["jid"];
			user.uin=(std::string)row["uin"];
			user.subscription=(std::string)row["subscription"];
			user.nickname=(std::string)row["nickname"];
			user.group=(std::string)row["group"];
			user.online=false;
			user.lastPresence="";
			rows[(std::string)row["uin"]]=user;
		}
	}
#else
	mysqlpp::StoreQueryResult::size_type i;
	mysqlpp::Row row;
	for (i = 0; i < res.num_rows(); ++i) {
		row = res[i];
		RosterRow user;
		user.id=(long) row["id"];
		user.jid=(std::string)row["jid"];
		user.uin=(std::string)row["uin"];
		user.subscription=(std::string)row["subscription"];
		user.nickname=(std::string)row["nickname"];
		user.group=(std::string)row["group"];
		if (user.subscription.empty())
			user.subscription="ask";
		user.online=false;
		user.lastPresence="";
		rows[(std::string)row["uin"]]=user;
	}
#endif
	return rows;
}

void SQLClass::getRandomStatus(std::string & status)
{
    mysqlpp::Query query = sql->query();
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res;
#else
	mysqlpp::StoreQueryResult res;
#endif
    mysqlpp::Row row;

    status.clear();

    query << "SELECT reklama FROM ad_statusy ORDER BY RAND() LIMIT 1";

    res = query.store();
#if MYSQLPP_HEADER_VERSION < 0x030000
    if (res) {
        if (row = res.fetch_row()) {
            status = (std::string)row["reklama"];
        }
    }
#else
    if (res.num_rows() > 0) {
        if (row = res[0]) {
            status = (std::string)row["reklama"];
        }
    }
#endif
}

// settings

void SQLClass::addSetting(const std::string &jid, const std::string &key, const std::string &value, PurpleType type) {
	mysqlpp::Query query = sql->query();
	query << "INSERT INTO "<< p->configuration().sqlPrefix <<"settings " << "(jid, var, type, value) VALUES (\"" << jid << "\",\"" << key << "\", \"" << type << "\", \"" << value << "\")";
	query.execute();
}

void SQLClass::updateSetting(const std::string &jid, const std::string &key, const std::string &value) {
	mysqlpp::Query query = sql->query();
	query << "UPDATE "<< p->configuration().sqlPrefix <<"settings SET value=\"" << value <<"\" WHERE jid=\"" << jid << "\" AND var=\"" << key << "\";";
	query.execute();
}

void SQLClass::getSetting(const std::string &jid, const std::string &key) {
	
}

GHashTable * SQLClass::getSettings(const std::string &jid) {
	GHashTable *settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
	PurpleType type;
	PurpleValue *value;
    mysqlpp::Query query = sql->query();
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res;
#else
	mysqlpp::StoreQueryResult res;
#endif
    mysqlpp::Row row;

    query << "SELECT * FROM "<< p->configuration().sqlPrefix <<"settings WHERE jid=\"" << jid << "\";";

    res = query.store();
	if (res) {
#if MYSQLPP_HEADER_VERSION < 0x030000
		mysqlpp::Row row;
		while(row = res.fetch_row()) {
#else
		mysqlpp::StoreQueryResult::size_type i;
		mysqlpp::Row row;
		for (i = 0; i < res.num_rows(); ++i) {
			row = res[i];
#endif
			type = (PurpleType) atoi(row["type"]);
			if (type == PURPLE_TYPE_BOOLEAN) {
				value = purple_value_new(PURPLE_TYPE_BOOLEAN);
				purple_value_set_boolean(value, atoi(row["value"]));
			}
			g_hash_table_replace(settings, g_strdup(row["var"]), value);
		}
	}



	return settings;
}

bool SQLClass::getVCard(const std::string &name, void (*handleTagCallback)(Tag *tag, Tag *user_data), Tag *user_data) {
    mysqlpp::Query query = sql->query();
#if MYSQLPP_HEADER_VERSION < 0x030000
	mysqlpp::Result res;
#else
	mysqlpp::StoreQueryResult res;
#endif
    mysqlpp::Row row;

    query << "SELECT vcard FROM "<< p->configuration().sqlPrefix <<"vcards WHERE username=\"" +name+ "\" AND DATE_ADD(timestamp, INTERVAL 1 DAY);";

    res = query.store();
	if (res) {
#if MYSQLPP_HEADER_VERSION < 0x030000
		mysqlpp::Row row;
		row = res.fetch_row();
		if (row) {
#else
		mysqlpp::StoreQueryResult::size_type i;
		mysqlpp::Row row;
		if (res.num_rows()!=0) {
			row = res[0];
#endif
			std::string vcardTag = (std::string) row["vcard"];
			p->parser()->getTag(vcardTag, handleTagCallback, user_data);
			return true;
		}
	}
	return false;
}

void SQLClass::updateVCard(const std::string &name, const std::string &vcard) {
	mysqlpp::Query query = sql->query();
	query << "INSERT INTO "<< p->configuration().sqlPrefix <<"vcards (username, vcard) VALUES (\"" << name <<"\",\"" << vcard <<"\") ON DUPLICATE KEY UPDATE vcard=\""+ vcard +"\";";
	query.execute();
}

// SQLClass::~SQLClass(){
//  	delete(sql);
//  	sql=NULL;
// }


