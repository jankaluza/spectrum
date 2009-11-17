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
#include <gloox/siprofileft.h>
#include <glib.h>
#include "purple.h"
#include "account.h"

class GlooxMessageHandler;
class FiletransferRepeater;
class RosterManager;

class RosterRow;

using namespace gloox;

typedef enum { 	CALLER_ADHOC,
				CALLER_SEARCH
				} AdhocDataCallerType;

struct AdhocData {
	std::string id;
	std::string from;
	std::string node;
	AdhocDataCallerType callerType;
};

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

struct Resource {
	int priority;
	std::string capsVersion;
	std::string name;
	operator bool() const {
		return !name.empty();
	}
};

struct Conversation {
	PurpleConversation *conv;
	std::string resource;
};

class User;

struct SaveData {
	User *user;
	long *id;
};

class User {
	public:
		User(GlooxMessageHandler *parent, JID jid, const std::string &username, const std::string &password, const std::string &userKey, long id);
		~User();

		void connect();
		void sendRosterX();
		void syncContacts();
		bool hasTransportFeature(int feature); // TODO: move me to p->hasTransportFeature and rewrite my API

		// Utils
		bool syncCallback();
		bool isInRoster(const std::string &name, const std::string &subscription);
		bool isOpenedConversation(const std::string &name);
		bool hasFeature(int feature, const std::string &resource = "");

		// XMPP stuff
		Tag *generatePresenceStanza(PurpleBuddy *buddy);

		// Libpurple stuff
		void purpleReauthorizeBuddy(PurpleBuddy *buddy);

		// Gloox callbacks
		void receivedPresence(const Presence &presence);
		void receivedSubscription(const Subscription &subscription);
		void receivedMessage(const Message& msg);
		void receivedChatState(const std::string &uin, const std::string &state);

		// Libpurple callbacks
		void purpleBuddyChanged(PurpleBuddy *buddy);
		void purpleBuddyRemoved(PurpleBuddy *buddy);
		void purpleBuddyCreated(PurpleBuddy *buddy);
		void purpleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status);
		void purpleBuddySignedOn(PurpleBuddy *buddy);
		void purpleBuddySignedOff(PurpleBuddy *buddy);
		void purpleMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags);
		void purpleConversationWriteIM(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);
		void purpleConversationWriteChat(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime);
		void purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic);
		void purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias);
		void purpleChatRemoveUsers(PurpleConversation *conv, GList *users);
		void purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals);
		void purpleAuthorizeReceived(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data);
		void purpleBuddyTypingStopped(const std::string &uin);
		void purpleBuddyTyping(const std::string &uin);
		void connected();
		void disconnected();

		// Filetransfers
		void addFiletransfer( const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size );
		void addFiletransfer( const JID& from );
		FiletransferRepeater* removeFiletransfer(const std::string &from) { FiletransferRepeater *repeater = (FiletransferRepeater *) g_hash_table_lookup(m_filetransfers, from.c_str()); if (repeater) g_hash_table_remove(m_filetransfers, from.c_str()); return repeater; }
		FiletransferRepeater* getFiletransfer(const std::string &from) { FiletransferRepeater *repeater = (FiletransferRepeater *) g_hash_table_lookup(m_filetransfers, from.c_str()); return repeater; }
		std::string actionData;

		// Connected
		bool isConnected() { return m_connected; }

		// Settings
		PurpleValue *getSetting(const char *key);
		void updateSetting(const std::string &key, PurpleValue *value);

		// Authorization requests
		bool hasAuthRequest(const std::string &name);
		void removeAuthRequest(const std::string &name);

		// Storage
		void storeBuddy(PurpleBuddy *buddy);

		// bind IP
		void setBindIP(const std::string& bindIP) { m_bindIP = bindIP; }

		// connection start
		time_t connectionStart() { return m_connectionStart; }

		void setResource(const std::string &resource, int priority = -256, const std::string &caps = "") {
			if (priority != -256) m_resources[resource].priority = priority;
			if (!caps.empty()) m_resources[resource].capsVersion = caps;
			m_resources[resource].name = resource;
		}
		void setActiveResource(const std::string &resource) { m_resource = resource; }
		bool hasResource(const std::string &r) {return m_resources.find(r) != m_resources.end(); }
		Resource & getResource(const std::string &r = "") { if (r.empty()) return m_resources[m_resource]; else return m_resources[r];}
		Resource & findResourceWithFeature(int feature);

		PurpleAccount *account() { return m_account; }
		std::map<std::string,Resource> & resources() { return m_resources; }
		int reconnectCount() { return m_reconnectCount; }
		bool isVIP() { return m_vip; }
		bool readyForConnect() { return m_readyForConnect; }
		void setReadyForConnect(bool ready) { m_readyForConnect = ready; }
		const std::string & username() { return m_username; }
		const std::string & jid() { return m_jid; }
		const std::string & resource() { return m_resource; }
		AdhocData & adhocData() { return m_adhocData; }
		void setAdhocData(AdhocData data) { m_adhocData = data; }
		std::map<std::string,RosterRow> & roster() { return m_roster; }
		const char *getLang() { return m_lang; }
		void setLang(const char *lang) { m_lang = lang; }
		GHashTable *settings() { return m_settings; }

		GlooxMessageHandler *p;
		const std::string & userKey() { return m_userKey; }
		void setProtocolData(void *protocolData) { m_protocolData = protocolData; }
		void *protocolData() { return m_protocolData; }
		GHashTable *mucs() { return m_mucs; }
		std::map<std::string,Conversation> conversations() { return m_conversations; }
		void setFeatures(int f) { m_features = f; }
		long storageId() { return m_userID; }
		bool loadingBuddiesFromDB() { return m_loadingBuddiesFromDB; }
		GHashTable *storageCache() { return m_storageCache; }
		void removeStorageTimer() { m_storageTimer = 0; }

		guint removeTimer;

	private:
		std::string m_userKey;
		PurpleAccount *m_account;	// PurpleAccount to which this user is connected
		void *m_protocolData;
		guint m_syncTimer;			// timer used for syncing purple buddy list and roster
		int m_subscribeLastCount;	// number of buddies which was in subscribeCache in previous iteration of m_syncTimer
		bool m_vip;					// true if the user is VIP
		bool m_readyForConnect;		// true if the user user wants to connect and we're ready to do it
		bool m_rosterXCalled;		// true if we are counting buddies for roster X
		bool m_connected;			// true if this user is connected to legacy account
		bool m_reconnectCount;		// number of passed reconnect tries
		AdhocData m_adhocData;		// name of node for AdhocHandler
		std::string m_bindIP;		// IP address to which libpurple will be binded
		std::string m_password;		// password used to connect to legacy network
		std::string m_username;		// legacy network user name
		std::string m_jid;			// Jabber ID of this user
		std::string m_resource;		// active resource
		const char *m_lang;			// xml:lang
		int m_features;
		time_t m_connectionStart;	// connection start timestamp
		GHashTable *m_mucs;			// MUCs
		GHashTable *m_filetransfers;
		std::map<std::string,RosterRow> m_roster;	// jabber roster of this user
		std::map<std::string,Resource> m_resources;	// list of all resources which are connected to the transport
		std::map<std::string,authRequest> m_authRequests;	// list of authorization requests (holds callbacks and user data)
		std::map<std::string,Conversation> m_conversations; // list of opened conversations
		std::map<std::string,PurpleBuddy *> m_subscribeCache;	// cache for contacts for roster X
		GHashTable *m_settings;		// user settings
		GHashTable *m_storageCache;	// cache for storing PurpleBuddies
		guint m_storageTimer;		// timer for storing
		long m_userID;				// userID for Database
		bool m_loadingBuddiesFromDB;
};

#endif
