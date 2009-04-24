#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <string>
#include <map>
#include "account.h"
#include "user.h"

class GlooxMessageHandler;

class UserManager
{
	public:
		UserManager(GlooxMessageHandler *m);
		~UserManager();
		User *getUserByJID(std::string barejid);
		User *getUserByAccount(PurpleAccount *account);
		void addUser(User *user) { m_users[user->jid()] = user; }
		void removeUser(User *user);
		void removeUserTimer(User *user);
		long onlineUserCount();
		int userCount() { return m_users.size(); }
	
	private:
		GlooxMessageHandler *main;
		std::map<std::string, User*> m_users;
	
};

#endif
