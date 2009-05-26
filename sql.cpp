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
	mysqlpp::Result res;
	mysqlpp::Row myrow;
	query << "SELECT COUNT(jid) as is_vip FROM `users` WHERE jid='"<< jid <<"' and expire>NOW();";
	res = query.store();
	myrow = res.fetch_row();
	if (int(myrow.at(0))==0)
		return false;
	else
		return true;
}

long SQLClass::getRegisteredUsersCount(){
	mysqlpp::Query query = sql->query();
	mysqlpp::Result res;
	mysqlpp::Row myrow;
	query << "select count(*) as count from "<< p->configuration().sqlPrefix <<"users";
	res = query.store();
	if (res){
		myrow = res.fetch_row();
		return long(myrow.at(0));
	}
	else return 0;
}

long SQLClass::getRegisteredUsersRosterCount(){
	mysqlpp::Query query = sql->query();
	mysqlpp::Result res;
	mysqlpp::Row myrow;
	query << "select count(*) as count from "<< p->configuration().sqlPrefix <<"rosters";
	res = query.store();
	if (res){
		myrow = res.fetch_row();
		return long(myrow.at(0));
	}
	else return 0;
}

void SQLClass::updateUserPassword(const std::string &jid,const std::string &password){
	mysqlpp::Query query = sql->query();
	query << "UPDATE "<< p->configuration().sqlPrefix <<"users SET password=\"" << password <<"\" WHERE jid=\"" << jid << "\";";
	query.execute();
}

void SQLClass::removeUINFromRoster(const std::string &jid,const std::string &uin){
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

void SQLClass::addUser(const std::string &jid,const std::string &uin,const std::string &password){
	mysqlpp::Query query = sql->query();
	query << "INSERT INTO "<< p->configuration().sqlPrefix <<"users " << "(id, jid, uin, password) VALUES (\"\",\"" << jid << "\",\"" << uin << "\", \"" << password << "\")";
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
	mysqlpp::Result res = query.store();
	user.id=-1;
	user.jid="";
	user.uin="";
	user.password="";
	if (res.size()) {
		mysqlpp::Row row = res.fetch_row();
		user.id=(long) row["id"];
		user.jid=(std::string)row["jid"];
		user.uin=(std::string)row["uin"];
		user.password=(std::string)row["password"];
		user.category=(int) row["group"];
	}
	return user;
}

std::map<std::string,RosterRow> SQLClass::getRosterByJid(const std::string &jid){
	std::map<std::string,RosterRow> rows;
	mysqlpp::Query query = sql->query();
	query << "SELECT * FROM "<< p->configuration().sqlPrefix <<"rosters WHERE jid=\"" << jid << "\";";
	mysqlpp::Result res = query.store();
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
	return rows;
}

std::map<std::string,RosterRow> SQLClass::getRosterByJidAsk(const std::string &jid){
	std::map<std::string,RosterRow> rows;
	mysqlpp::Query query = sql->query();
	query << "SELECT * FROM "<< p->configuration().sqlPrefix <<"rosters WHERE jid=\"" << jid << "\" AND subscription=\"ask\";";
	mysqlpp::Result res = query.store();
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
	return rows;
}

void SQLClass::getRandomStatus(std::string & status)
{
    mysqlpp::Query query = sql->query();
    mysqlpp::Result res;
    mysqlpp::Row row;

    status.clear();

    query << "SELECT reklama FROM ad_statusy ORDER BY RAND() LIMIT 1";

    res = query.store();

    if (res) {
        if (row = res.fetch_row()) {
            status = (std::string)row["reklama"];
        }
    }
}

			
// SQLClass::~SQLClass(){
//  	delete(sql);
//  	sql=NULL;
// }


