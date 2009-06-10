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

#ifndef _HI_MAIN_H
#define _HI_MAIN_H

#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <mysql++.h>

#include <gloox/component.h>
#include <gloox/messagehandler.h>
#include <gloox/connectiontcpclient.h>
#include <gloox/disco.h>
#include <gloox/connectionlistener.h>
#include <gloox/presencehandler.h>
#include <gloox/subscriptionhandler.h>
#include <gloox/socks5bytestreamserver.h>
#include <gloox/siprofileft.h>
#include <gloox/message.h>

#include <glib.h>

#include "account.h"
#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "ft.h"
#include "notify.h"
#include "prefs.h"
#include "prpl.h"
#include "pounce.h"
#include "savedstatuses.h"
#include "sound.h"
#include "status.h"
#include "util.h"
#include "whiteboard.h"
#include "request.h"
#include "ft.h"

#include <libintl.h>
#include <locale.h>

#define HIICQ_UI "hiicq"
#define tr(lang,STRING)    localization.translate(lang,STRING)
#define _(STRING)    STRING

#include "registerhandler.h"
#include "discoinfohandler.h"
#include "xpinghandler.h"
#include "statshandler.h"
#include "vcardhandler.h"
#include "gatewayhandler.h"
#include "caps.h"
#include "sql.h"
#include "user.h"
#include "filetransfermanager.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "localization.h"


using namespace gloox;

extern Localization localization;

class GlooxDiscoHandler;
class GlooxDiscoInfoHandler;
class GlooxRegisterHandler;
class GlooxXPingHandler;
class GlooxStatsHandler;
class GlooxVCardHandler;
class GlooxGatewayHandler;
class GlooxAdhocHandler;
class SQLClass;
class FileTranferManager;
class UserManager;
class AbstractProtocol;
// class AdhocHandler;
class AdhocRepeater;

struct User;
struct UserRow;
struct authData;

struct replaceBadJidCharacters {
	void operator()(char& c) { if(c == '@') c = '%'; }
};

struct replaceJidCharacters {
	void operator()(char& c) { if(c == '%') c = '@'; }
};

typedef enum { 	TRANSPORT_FEATURE_TYPING_NOTIFY = 2,
				TRANSPORT_FEATURE_AVATARS = 4,
                TRANSPORT_MANGLE_STATUS = 8,
				} TransportFeatures;

struct fileTransferData{
	std::string from;
	std::string to;
	std::string filename;
	std::string name;
};

struct Configuration {
	std::string discoName;	// name which will be shown in service discovery
	std::string protocol;	// protocol used for transporting
	std::string server;		// address of server to bind to
	std::string password;	// server password
	std::string jid;		// JID of this transport
	int port;				// server port
	
	bool onlyForVIP;		// true if transport is only for users in VIP users database
	std::map<int,std::string> bindIPs;	// IP address to which libpurple should bind connections
	
	std::string userDir;	// directory used as .tmp directory for avatars and other libpurple stuff
	std::string filetransferCache;	// directory where files are saved
	std::string base64Dir;	// TODO: I'm depracted, remove me

	std::string sqlHost;	// mysql host
	std::string sqlPassword;	// mysql password
	std::string sqlUser;	// mysql user
	std::string sqlDb;		// mysql database
	std::string sqlPrefix;	// mysql prefix used for tables
};

class GlooxMessageHandler : public MessageHandler,ConnectionListener,PresenceHandler,SubscriptionHandler
{

public:
	GlooxMessageHandler();
	~GlooxMessageHandler();
	
	static GlooxMessageHandler *instance() { return m_pInstance; }
	
	// Purple related
	void purpleBuddyChanged(PurpleBuddy* buddy);
	void purpleConnectionError(PurpleConnection *gc,PurpleConnectionError reason,const char *text);
	void purpleMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags);
	void purpleConversationWriteIM(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime);
	void purpleConversationWriteChat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime);
	void purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals);
	void purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias);
	void purpleChatRemoveUsers(PurpleConversation *conv, GList *users);
	void * purpleAuthorizeReceived(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data);
	void purpleBuddyTyping(PurpleAccount *account, const char *who);
	void purpleBuddyTypingStopped(PurpleAccount *account, const char *who);
	void purpleBuddyRemoved(PurpleBuddy *buddy);
	void purpleAuthorizeClose(void *data);
	void purpleFileReceiveRequest(PurpleXfer *xfer);
	void purpleFileReceiveComplete(PurpleXfer *xfer);

	// MessageHandler
	void handleMessage (const Message &msg, MessageSession *session=0);

	// ConnectionListener
	void onConnect();
	void onDisconnect(ConnectionError e);
	void onSessionCreateError  	(   	SessionCreateError   	 error);
	bool onTLSConnect(const CertInfo & info);
	
	// Gloox handlers
	void handlePresence(const Presence &presence);
	void handleSubscription(const Subscription &stanza);
	void signedOn(PurpleConnection *gc,gpointer unused);
	
	// User related
	// maybe we should move it to something like UserManager
	bool hasCaps(const std::string &name);
	void removeUser(User *user);
	
	UserManager *userManager() { return m_userManager; }
	GlooxStatsHandler *stats() { return m_stats; }
	Configuration configuration() { return m_configuration; }
	std::string jid() { return m_configuration.jid; } // just to create shortcut and because of historical reasons
	SQLClass *sql() { return m_sql; }
	GlooxVCardHandler *vcard() { return m_vcard; }
	AbstractProtocol *protocol() { return m_protocol; }
	GlooxAdhocHandler *adhoc() { return m_adhoc; }
	
	FileTransferManager* ftManager;
	SIProfileFT* ft;
	
	Component *j;
// 	std::vector<User*> *users;
	int lastIP;
	std::map <std::string,int> capsCache;
	
	
	
	GlooxGatewayHandler *gatewayHandler;
	SOCKS5BytestreamServer* ftServer;
	
private:
// 	bool callback(GIOCondition condition);
	bool initPurple();
	void loadConfigFile();
	void loadProtocol();
	
	Configuration m_configuration;
	AbstractProtocol *m_protocol;

	SQLClass *m_sql;
	
	GlooxDiscoHandler *m_discoHandler;
	GlooxDiscoInfoHandler *m_discoInfoHandler;
 	GlooxRegisterHandler *m_reg;
	GlooxXPingHandler *m_xping;
	GlooxStatsHandler *m_stats;
	GlooxVCardHandler *m_vcard;
// 	std::list <GlooxAdhocHandler *> m_adhoc_handlers;
	GlooxAdhocHandler *m_adhoc;
	
	GIOChannel *connectIO;
	
	UserManager *m_userManager;
	static GlooxMessageHandler* m_pInstance;
	bool m_firstConnection;

	
	
};

#endif
