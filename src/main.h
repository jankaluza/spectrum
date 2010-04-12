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
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>

#include <glib.h>

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
#include <gloox/eventhandler.h>
#include <gloox/vcardhandler.h>
#include <gloox/vcardmanager.h>

#include "purple.h"
#include "account.h"
#include "conversation.h"
#include "core.h"
#include "debug.h"
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
#include "request.h"
#include "ft.h"

#ifndef WIN32
#include <libintl.h>
#include <locale.h>
#endif

#include "transport_config.h"
#include "localization.h"
#include "configfile.h"

#define PURPLE_UI "spectrum"
#define tr(lang,STRING)    localization.translate(lang,STRING)
#define _(STRING)    STRING

using namespace gloox;

extern Localization localization;
extern LogClass Log_;

class CapabilityHandler;
class GlooxDiscoInfoHandler;
class GlooxRegisterHandler;
class GlooxXPingHandler;
class GlooxStatsHandler;
class GlooxVCardHandler;
class GlooxGatewayHandler;
class GlooxAdhocHandler;
class SQLClass;
class FileTransferManager;
class UserManager;
class AbstractProtocol;
class AdhocRepeater;
class GlooxSearchHandler;
class GlooxParser;
class AccountCollector;
class Transport;
class SpectrumNodeHandler;
#ifndef WIN32
class ConfigInterface;
#endif

struct User;
struct UserRow;
struct authData;

				
/*
 * Enum used by gloox::StanzaExtension to identify StanzaExtension by Gloox.
 */
typedef enum {	ExtGateway = 1024,
				ExtStats = 1025
} MyStanzaExtensions;

/*
 * Main transport class. It inits libpurple and Gloox, runs event loop and handles almost all signals.
 * This class is created only once and can be reached by static GlooxMessageHandler::instance() function.
 */
class GlooxMessageHandler : public MessageHandler, ConnectionListener, PresenceHandler, SubscriptionHandler, LogHandler, public EventHandler, public VCardHandler {
public:
	GlooxMessageHandler(const std::string &config);
	~GlooxMessageHandler();

	/*
	 * Returns pointer to GlooxMessageHandler class.
	 */
	static GlooxMessageHandler *instance() { return m_pInstance; }

	/*
	 * Connects transport to Jabber server. It's called in constructor and in onDisconnect function
	 * when transport disconnects from server.
	 */
	void transportConnect();

	/*
	 * Callbacks for libpurple UI-ops.
	 */
	void purpleConnectionError(PurpleConnection *gc, PurpleConnectionError reason, const char *text);
	void purpleMessageReceived(PurpleAccount* account, char * name, char *msg, PurpleConversation *conv, PurpleMessageFlags flags);
	void purpleConversationWriteIM(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime);
	void purpleConversationWriteChat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime);
	void purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic);
	void purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals);
	void purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias);
	void purpleChatRemoveUsers(PurpleConversation *conv, GList *users);
	void * purpleAuthorizeReceived(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data);
	void purpleBuddyTyping(PurpleAccount *account, const char *who);
	void purpleBuddyTypingPaused(PurpleAccount *account, const char *who);
	void purpleBuddyTypingStopped(PurpleAccount *account, const char *who);
	void purpleBuddyRemoved(PurpleBuddy *buddy);
	void purpleBuddyCreated(PurpleBuddy *buddy);
	void purpleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status);
	void purpleBuddySignedOn(PurpleBuddy *buddy);
	void purpleBuddySignedOff(PurpleBuddy *buddy);
	void purpleAuthorizeClose(void *data);
	void notifyEmail(PurpleConnection *gc, const char *subject, const char *from, const char *to, const char *url);
	void signedOn(PurpleConnection *gc, gpointer unused);

	/*
	 * Gloox callbacks
	 */
	void onConnect();
	void onDisconnect(ConnectionError e);
	void onSessionCreateError(const Error *error);
	bool onTLSConnect(const CertInfo & info);
	
	void handleMessage (const Message &msg, MessageSession *session = 0);
	void handlePresence(const Presence &presence);
	void handleSubscription(const Subscription &stanza);
	void handleLog(LogLevel level, LogArea area, const std::string &message);
	void handleEvent (const Event &event) {}
	void handleVCard(const JID& jid, const VCard* vcard);
	void handleVCardResult(VCardContext context, const JID& jid, StanzaError se);
	void fetchVCard(const std::string &jid) { m_vcardManager->fetchVCard(jid, this); }

	UserManager *userManager() { return m_userManager; }
	GlooxStatsHandler *stats() { return m_stats; }
	Configuration & configuration() { return m_configuration; }
	const std::string & jid() { return m_configuration.jid; } // just to create shortcut and because of historical reasons
	SQLClass *sql() { return m_sql; }
	GlooxVCardHandler *vcard() { return m_vcard; }
	AbstractProtocol *protocol() { return m_protocol; }
	GlooxAdhocHandler *adhoc() { return m_adhoc; }
	GlooxSearchHandler *searchHandler() { return m_searchHandler; }
	GlooxParser *parser() { return m_parser; }
	AccountCollector *collector() { return m_collector; }

	// TODO: Make me private!
	FileTransferManager* ftManager;
	SIProfileFT* ft;
	Component *j;
	int lastIP;
	GlooxGatewayHandler *gatewayHandler;
	SOCKS5BytestreamServer* ftServer;
	GMainLoop *loop() { return m_loop; }
	bool loadConfigFile(const std::string &config = "");

private:
	/*
	 * Inits libpurple and PurpleCmd API. Returns true if libpurple was sucefully loaded.
	 */
	bool initPurple();

	/*
	 * Loads m_protocol class. Returns true if protocol is loaded.
	 */
	bool loadProtocol();

	Configuration m_configuration;				// configuration struct
	AbstractProtocol *m_protocol;				// currently used protocol
	SQLClass *m_sql;							// storage class
	AccountCollector *m_collector;

	CapabilityHandler *m_discoHandler;			// disco stanza handler
	GlooxDiscoInfoHandler *m_discoInfoHandler;	// disco#info handler
 	GlooxRegisterHandler *m_reg;				// jabber:iq:register handler
	GlooxStatsHandler *m_stats;					// Statistic Gathering handler (xep-0039)
	GlooxVCardHandler *m_vcard;					// VCard handler
	GlooxSearchHandler *m_searchHandler;		// jabber:iq:search handler
	GlooxAdhocHandler *m_adhoc;					// Ad-Hoc commands handler
	GlooxParser *m_parser;						// Gloox parser - makes Tag* from std::string
	VCardManager* m_vcardManager;
	SpectrumNodeHandler *m_spectrumNodeHandler;
#ifndef WIN32
	ConfigInterface *m_configInterface;
#endif
	GIOChannel *connectIO;						// GIOChannel for Gloox socket
	guint connectID;

	UserManager *m_userManager;					// UserManager class
	bool m_firstConnection;						// true if transporConnect is called for first time
	static GlooxMessageHandler* m_pInstance;
	Transport *m_transport;
	GMainLoop *m_loop;
	std::string m_config;
};

#endif
