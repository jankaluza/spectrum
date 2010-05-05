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
#include "transport.h"

static gboolean deleteUser(gpointer data){
	AbstractUser *user = (AbstractUser*) data;
	Transport::instance()->sql()->setUserOnline(user->storageId(), false);
	delete user;
	Log("logout", "delete user; called => user is sucesfully removed");
	return FALSE;
}

static gboolean removeUserCallback(gpointer key, gpointer v, gpointer data) {
	AbstractUser *user = (AbstractUser *) v;
	Log("logout", "Removing user " << user->jid());
	Tag *tag = new Tag("presence");
	tag->addAttribute("to", user->jid());
	tag->addAttribute("type", "unavailable");
	tag->addAttribute("from", Transport::instance()->jid());
	Transport::instance()->send(tag);
	delete user;
	g_usleep(10000);
	return TRUE;
}

UserManager::UserManager() {
	m_users = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	m_cachedUser = NULL;
	m_onlineBuddies = 0;
}

UserManager::~UserManager(){
	g_hash_table_foreach_remove(m_users, removeUserCallback, NULL);
	g_hash_table_destroy(m_users);
}

AbstractUser *UserManager::getUserByJID(std::string barejid){
	if (m_cachedUser && barejid == m_cachedUser->userKey()) {
		return m_cachedUser;
	}
	AbstractUser *user = (AbstractUser*) g_hash_table_lookup(m_users, barejid.c_str());
	m_cachedUser = user;
	return user;
}

AbstractUser *UserManager::getUserByAccount(PurpleAccount * account){
	if (account == NULL) return NULL;
	return (AbstractUser *) account->ui_data;
}

void UserManager::addUser(AbstractUser *user) {
	g_hash_table_replace(m_users, g_strdup(user->userKey().c_str()), user);
}

void UserManager::removeUser(AbstractUser *user){
	Log("logout", "removing user");
	g_hash_table_remove(m_users, user->userKey().c_str());
	if (m_cachedUser && user->userKey() == m_cachedUser->userKey()) {
		m_cachedUser = NULL;
	}
	if (user->removeTimer != 0)
		purple_timeout_remove(user->removeTimer);
	Transport::instance()->sql()->setUserOnline(user->storageId(), false);
	delete user;
	Log("logout", "delete user; called => user is sucesfully removed");
}

void UserManager::removeUserTimer(AbstractUser *user){
	Log("logout", "removing user by timer");
	g_hash_table_remove(m_users, user->userKey().c_str());
	if (m_cachedUser && user->userKey() == m_cachedUser->userKey()) {
		m_cachedUser = NULL;
	}
	// this will be called by gloop after all
	user->removeTimer = purple_timeout_add_seconds(1,&deleteUser,user);
}

void UserManager::buddyOnline() {
	m_onlineBuddies++;
}

void UserManager::buddyOffline() {
	m_onlineBuddies--;
}

long UserManager::onlineBuddiesCount() {
	return m_onlineBuddies;
}

int UserManager::userCount() {
	return g_hash_table_size(m_users);
}

void UserManager::removeAllUsers() {
	g_hash_table_foreach_remove(m_users, removeUserCallback, NULL);
	m_cachedUser = NULL;
}
