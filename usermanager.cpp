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
	m_users = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	m_cachedUser = NULL;
}

UserManager::~UserManager(){
	g_hash_table_destroy(m_users);
}

User *UserManager::getUserByJID(std::string barejid){
	if (m_cachedUser && barejid == m_cachedUser->userKey()) {
		return m_cachedUser;
	}
	User *user = (User*) g_hash_table_lookup(m_users, barejid.c_str());
	m_cachedUser = user;
	return user;
}

User *UserManager::getUserByAccount(PurpleAccount * account){
	if (account == NULL) return NULL;
	std::string jid(purple_account_get_string(account,"lastUsedJid",""));
	return getUserByJID(jid);
}

void UserManager::removeUser(User *user){
	Log().Get("logout") << "removing user";
	g_hash_table_remove(m_users, user->userKey().c_str());
	if (m_cachedUser && user->userKey() == m_cachedUser->userKey()) {
		m_cachedUser = NULL;
	}
	if (user->removeTimer != 0)
		purple_timeout_remove(user->removeTimer);
	delete user;
	Log().Get("logout") << "delete user; called => user is sucesfully removed";
}

void UserManager::removeUserTimer(User *user){
	Log().Get("logout") << "removing user by timer";
	g_hash_table_remove(m_users, user->userKey().c_str());
	if (m_cachedUser && user->userKey() == m_cachedUser->userKey()) {
		m_cachedUser = NULL;
	}
	// this will be called by gloop after all
	user->removeTimer = purple_timeout_add(0,&deleteUser,user);
}

void UserManager::buddyOnline() {
	m_onlineBuddies++;
}

void UserManager::buddyOffline() {
	m_onlineBuddies--;
}

long UserManager::onlineUserCount(){
	return m_onlineBuddies;
}
