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
