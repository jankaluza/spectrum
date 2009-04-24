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

#include <mysql++.h>
#include <gloox/clientbase.h>
#include <glib.h>
class GlooxMessageHandler;
#include "main.h"
using namespace gloox;

struct UserRow {
	long id;
	std::string jid;
	std::string uin;
	std::string password;
	int category;
};

struct RosterRow {
	long id;
	std::string jid;
	std::string uin;
	std::string subscription;
	bool online;
	std::string lastPresence;
};


class SQLClass
{

public:
	SQLClass(GlooxMessageHandler *parent);
// 	~SQLClass();
	void addUser(const std::string &jid,const std::string &uin,const std::string &password);
	void addDownload(const std::string &filename,const std::string &vip);
	void removeUser(const std::string &jid);
	void updateUserPassword(const std::string &jid,const std::string &password);
	void removeUserFromRoster(const std::string &jid);
	void addUserToRoster(const std::string &jid,const std::string &uin,const std::string subscription);
	void updateUserRosterSubscription(const std::string &jid,const std::string &uin,const std::string subscription);
	void removeUINFromRoster(const std::string &jid,const std::string &uin);
	bool isVIP(const std::string &jid);
	long getRegisteredUsersCount();
	long getRegisteredUsersRosterCount();
    void getRandomStatus(std::string & status);
	UserRow getUserByJid(const std::string &jid);
	std::map<std::string,RosterRow> getRosterByJid(const std::string &jid);
	std::map<std::string,RosterRow> getRosterByJidAsk(const std::string &jid);
	mysqlpp::Connection *sql;
	mysqlpp::Connection *vipSQL;
	GlooxMessageHandler *p;
};

#endif
