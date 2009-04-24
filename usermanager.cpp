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

#include "usermanager.h"
#include "main.h"
#include "user.h"
#include "log.h"

static gboolean deleteUser(gpointer data){
	User *user = (User*) data;
	delete user;
	Log().Get("logout") << "delete user; called => user is sucesfully removed";
	return FALSE;
}

UserManager::UserManager(GlooxMessageHandler *m){
	main = m;
}

UserManager::~UserManager(){
	
}

User *UserManager::getUserByJID(std::string barejid){
	User *user = NULL;
	std::map<std::string,User*>::iterator it = m_users.find(barejid);
	if (it != m_users.end())
		user = (*it).second;
	return user;
}

User *UserManager::getUserByAccount(PurpleAccount * account){
	if (account == NULL) return NULL;
	std::string jid(purple_account_get_string(account,"lastUsedJid",""));
	return getUserByJID(JID(jid).bare());
}

void UserManager::removeUser(User *user){
	Log().Get("logout") << "removing user";
	m_users.erase(user->jid());
	delete user;
	Log().Get("logout") << "delete user; called => user is sucesfully removed";
}

void UserManager::removeUserTimer(User *user){
	Log().Get("logout") << "removing user by timer";
	m_users.erase(user->jid());
	// this will be called by gloop after all
	g_timeout_add(0,&deleteUser,user);
}

long UserManager::onlineUserCount(){
	long users=0;

	for(std::map<std::string,User*>::iterator it = m_users.begin() ; it != m_users.end() ; it++) {
		for(std::map<std::string, RosterRow>::iterator u = (*it).second->roster().begin(); u != (*it).second->roster().end() ; u++){
			if ((*u).second.online==true){
				users+=1;
			}
		}
	}
	return users;
}
