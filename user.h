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

#ifndef _HI_USER_H
#define _HI_USER_H

#include <time.h>
#include <gloox/clientbase.h>
#include <gloox/messagesession.h>
#include <glib.h>
class GlooxMessageHandler;

#include "main.h"
#include "sql.h"
#include "caps.h"

#include "account.h"
#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "eventloop.h"
#include "ft.h"
#include "log.h"
#include "notify.h"
#include "prefs.h"
#include "prpl.h"
#include "pounce.h"
#include "savedstatuses.h"
#include "sound.h"
#include "status.h"
#include "util.h"
#include "whiteboard.h"
#include "privacy.h"
#include "striphtmltags.h"

class RosterRow;

using namespace gloox;

struct authData{
	PurpleAccount *account;
	std::string who;
};

struct authRequest {
	PurpleAccountRequestAuthorizationCb authorize_cb;
	PurpleAccountRequestAuthorizationCb deny_cb;
	void *user_data;
};

struct subscribeContact {
	std::string uin;
	std::string alias;
	std::string	group;
};

class User
{

public:
	User(GlooxMessageHandler *parent, const std::string &jid, const std::string &username, const std::string &password);
	~User();
// 	void init(GlooxMessageHandler *parent);
	GlooxMessageHandler *p;
	bool syncCallback();
	bool connected;
	void receivedPresence(Stanza *stanza);
	void purpleBuddyChanged(PurpleBuddy *buddy);
	void purpleMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags);
	void purpleAuthorizeReceived(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data);
	Tag *generatePresenceStanza(PurpleBuddy *buddy);
	bool isInRoster(const std::string &name,const std::string &subscription);
	bool isOpenedConversation(const std::string &name);
	bool hasFeature(int feature);
	void receivedMessage( Stanza* stanza);
	void receivedChatState(const std::string &uin,const std::string &state);
	void purpleBuddyTypingStopped(const std::string &uin);
	void purpleBuddyTyping(const std::string &uin);
	void sendRosterX();
	void syncContacts();
	void remove();
	void connect();
	bool hasTransportFeature(int feature);
	void purpleReauthorizeBuddy(PurpleBuddy *buddy);

	std::string actionData;
	time_t connectionStart;
	
	int reconnectCount;
	int features;
	
	// Entity Capabilities
	std::string capsVersion() { return m_capsVersion; }
	void setCapsVersion(const std::string &capsVersion) { m_capsVersion = capsVersion; }
	
	// Authorization requests
	bool hasAuthRequest(const std::string &name);
	void removeAuthRequest(const std::string &name);
	
	// bind IP
	void setBindIP(const std::string& bindIP) { m_bindIP = bindIP; }
	
	PurpleAccount *account() { return m_account; }
	std::map<std::string,int> resources() { return m_resources; }
	bool isVIP() { return m_vip; }
	bool readyForConnect() { return m_readyForConnect; }
	std::string username() { return m_username; }
	std::string jid() { return m_jid; }
	std::string resource() { return m_resource; }
	std::map<std::string,RosterRow> roster() { return m_roster; }
	
	private:
		PurpleAccount *m_account;	// PurpleAccount to which this user is connected
		guint m_syncTimer;			// timer used for syncing purple buddy list and roster
		int m_subscribeLastCount;	// number of buddies which was in subscribeCache in previous iteration of m_syncTimer
		bool m_vip;					// true if the user is VIP
		bool m_readyForConnect;		// true if the user user wants to connect and we're ready to do it
		bool m_rosterXCalled;		// true if we are counting buddies for roster X
		std::string m_bindIP;		// IP address to which libpurple will be binded
		std::string m_password;		// password used to connect to legacy network
		std::string m_username;		// legacy network user name
		std::string m_jid;			// Jabber ID of this user
		std::string m_resource;		// active resource
		std::string m_capsVersion;	// caps version of client which connected as first (TODO: this should be changed with active resource)
		std::map<std::string,RosterRow> m_roster;	// jabber roster of this user
		std::map<std::string,int> m_resources;	// list of all resources which are connected to the transport
		std::map<std::string,authRequest> m_authRequests;	// list of authorization requests (holds callbacks and user data)
		std::map<std::string,PurpleConversation *> m_conversations; // list of opened conversations
		std::vector<subscribeContact> m_subscribeCache;	// cache for contacts for roster X
};

#endif
