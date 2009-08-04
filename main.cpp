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

#include "main.h"
#include "utf8.h"
#include "log.h"
#include "geventloop.h"
#include "autoconnectloop.h"
#include "usermanager.h"
#include "adhochandler.h"
#include "adhocrepeater.h"
#include "searchhandler.h"
#include "searchrepeater.h"
#include "filetransferrepeater.h"
#include "parser.h"
#include "commands.h"
#include "protocols/abstractprotocol.h"
#include "protocols/icq.h"
#include "protocols/facebook.h"
#include "protocols/gg.h"
#include "protocols/msn.h"
#include "protocols/irc.h"
#include "protocols/xmpp.h"
#include "protocols/myspace.h"
#include "protocols/qq.h"
#include "protocols/simple.h"
#include "protocols/aim.h"
#include "cmds.h"

#include <gloox/tlsbase.h>
#include <gloox/compressionbase.h>

namespace gloox {
	class HiComponent : public Component {
		public:
			HiComponent(const std::string & ns, const std::string & server, const std::string & component, const std::string & password, int port = 5347) : Component(ns, server, component, password, port) {};

			virtual ~HiComponent() {};

			// unfortunately we can't override const std::string variant
// 			virtual void send(Tag *tag) {
// 				std::string tagxml;
// 				std::string outxml;
// 				if (!tag)
// 					return;
// 
// 				try {
// 					tagxml = tag->xml();
// 					outxml.resize(tagxml.size(), 0);
// 
// 					// replace invalid utf-8 characters
// 					utf8::replace_invalid(tagxml.begin(), tagxml.end(), outxml.begin(), '?');
// 
// 					// replace invalid xml characters
// 					for (std::string::iterator it = outxml.begin(); it != outxml.end(); ++it) {
// 						if (((unsigned char)*it) < 0x20 && (*it != 0x09 && *it != 0x0A && *it != 0x0D)) {
// 							*it = '?';
// 						}
// 					}
// 					send(outxml);
// 				} catch (...) {     // replace_invalid can throw exception? wtf!
// 				}
// 
// 				delete tag;
// 			}
// 		protected:
// 			void send( const std::string & xml) {    // copied from ClientBase::send(std::string), why have private function which handles protected data ??
// 				if( m_connection && m_connection->state() == StateConnected ) {
// 					if( m_compression && m_compressionActive )
// 						m_compression->compress( xml );
// 					else if( m_encryption && m_encryptionActive )
// 						m_encryption->encrypt( xml );
// 					else
// 						m_connection->send( xml );
// 				}
// 			}
	};
};

/*
 * New message from legacy network received (we can create conversation here)
 */
static void newMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags)
{
	GlooxMessageHandler::instance()->purpleMessageReceived(account,name,msg,conv,flags);
}

/*
 * Called when libpurple wants to write some message to Chat
 */
static void conv_write_im(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime)
{
	GlooxMessageHandler::instance()->purpleConversationWriteIM(conv,who,message,flags,mtime);
}

/*
 * Called when libpurple wants to write some message to Groupchat
 */
static void conv_write_chat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	GlooxMessageHandler::instance()->purpleConversationWriteChat(conv,who,message,flags,mtime);
}

/*
 * Called when libpurple wants to write some message to Conversation
 */
static void conv_write_conv(PurpleConversation *conv, const char *who, const char *alias,const char *message, PurpleMessageFlags flags, time_t mtime) {
	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		conv_write_im(conv, who, message, flags, mtime);
	}
	else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
		conv_write_chat(conv, who, message, flags, mtime);
	}
}

/*
 * Called when chat topic was changed
 */
static void conv_chat_topic_changed(PurpleConversation *chat, const char *who, const char *topic) {
	GlooxMessageHandler::instance()->purpleChatTopicChanged(chat, who, topic);
}

/*
 * Called when there are new users added
 */
static void conv_chat_add_users(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	GlooxMessageHandler::instance()->purpleChatAddUsers(conv, cbuddies, new_arrivals);
}

/*
 * Called when user is renamed
 */
static void conv_chat_rename_user(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	GlooxMessageHandler::instance()->purpleChatRenameUser(conv, old_name, new_name, new_alias);
}

/*
 * Called when there are is removed
 */
static void conv_chat_remove_users(PurpleConversation *conv, GList *users) {
	GlooxMessageHandler::instance()->purpleChatRemoveUsers(conv, users);
}

/*
 * Called when user is logged in...
 */
static void signed_on(PurpleConnection *gc,gpointer unused)
{
	GlooxMessageHandler::instance()->signedOn(gc,unused);
}

/*
 * Called when somebody from legacy network start typing
 */
static void buddyTyping(PurpleAccount *account, const char *who, gpointer null){
	GlooxMessageHandler::instance()->purpleBuddyTyping(account,who);
}

/*
 * Called when somebody from legacy network stops typing
 */
static void buddyTypingStopped(PurpleAccount *account, const char *who, gpointer null){
	GlooxMessageHandler::instance()->purpleBuddyTypingStopped(account,who);
}

/*
 * Called when PurpleBuddy is removed
 */
static void buddyRemoved(PurpleBuddy *buddy, gpointer null) {
	GlooxMessageHandler::instance()->purpleBuddyRemoved(buddy);
}

/*
 * Called when purple disconnects from legacy network.
 */
void connection_report_disconnect(PurpleConnection *gc,PurpleConnectionError reason,const char *text){
	GlooxMessageHandler::instance()->purpleConnectionError(gc,reason,text);
}

/*
 * Called when purple wants to some input from transport.
 */
static void * requestInput(const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb,PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data){
	Log().Get("purple") << "new requestInput";
	if (primary) {
		std::string primaryString(primary);
		Log().Get("purple") << "primary string: " << primaryString;
		User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
		if (!user) return NULL;
		// Check if there is some adhocData. If it's there, this request will be handled by some handler registered to this data.
		if (!user->adhocData().id.empty()) {
			if (user->adhocData().callerType == CALLER_ADHOC) {
				AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primaryString, secondary ? std::string(secondary):std::string(), default_value ? std::string(default_value):std::string(), multiline, masked, ok_cb, cancel_cb, user_data);
				GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
				AdhocData data;
				data.id="";
				user->setAdhocData(data);
				return repeater;
			}
			else if (user->adhocData().callerType == CALLER_SEARCH) {
				SearchRepeater *repeater = new SearchRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primaryString, secondary ? std::string(secondary):std::string(), default_value ? std::string(default_value):std::string(), multiline, masked, ok_cb, cancel_cb, user_data);
				GlooxMessageHandler::instance()->searchHandler()->registerSession(user->adhocData().from, repeater);
// 				AdhocData data;
// 				data.id="";
// 				user->setAdhocData(data);
				return repeater;
			}
		}
		// emit protocol signal
		GlooxMessageHandler::instance()->protocol()->onPurpleRequestInput(user, title, primary, secondary, default_value, multiline, masked, hint, ok_text, ok_cb, cancel_text, cancel_cb, account, who, conv, user_data);
	}
	return NULL;
}

static void * notifySearchResults(PurpleConnection *gc, const char *title, const char *primary, const char *secondary, PurpleNotifySearchResults *results, gpointer user_data) {
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(purple_connection_get_account(gc));
	if (!user) return NULL;
	if (!user->adhocData().id.empty()) {
		if (user->adhocData().callerType == CALLER_SEARCH) {
			GlooxMessageHandler::instance()->searchHandler()->searchResultsArrived(user->adhocData().from, results);
			AdhocData data;
			data.id="";
			user->setAdhocData(data);
		}
	}

	return NULL;
}

static void *requestFields(const char *title, const char *primary, const char *secondary, PurpleRequestFields *fields, const char *ok_text, GCallback ok_cb, const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who, PurpleConversation *conv, void *user_data) {
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	if (user && !user->adhocData().id.empty()) {
		if (user->adhocData().callerType == CALLER_ADHOC) {
			AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primary ? std::string(primary):std::string(), secondary ? std::string(secondary):std::string(), fields, ok_cb, cancel_cb, user_data);
			GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
			AdhocData data;
			data.id="";
			user->setAdhocData(data);
			return repeater;
		}
		else if (user->adhocData().callerType == CALLER_SEARCH) {
			SearchRepeater *repeater = new SearchRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primary ? std::string(primary):std::string(), secondary ? std::string(secondary):std::string(), fields, ok_cb, cancel_cb, user_data);
			GlooxMessageHandler::instance()->searchHandler()->registerSession(user->adhocData().from, repeater);
			return repeater;
		}
	}

	return NULL;
}

static void * requestAction(const char *title, const char *primary,const char *secondary, int default_action,PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data,size_t action_count, va_list actions){
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	if (user && !user->adhocData().id.empty()) {
		AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primary ? std::string(primary):std::string(), secondary ? std::string(secondary):std::string(), default_action, user_data, action_count, actions);
		GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
		AdhocData data;
		data.id="";
		user->setAdhocData(data);
		return repeater;
	}
	else if (title){
		std::string headerString(title);
		Log().Get("purple") << "header string: " << headerString;
		if (headerString=="SSL Certificate Verification"){
			va_arg(actions, char *);
			((PurpleRequestActionCb) va_arg(actions, GCallback))(user_data,2);
		}
	}
	return NULL;
}

static void requestClose(PurpleRequestType type, void *ui_handle) {
	if (!ui_handle)
		return;
	if (type == PURPLE_REQUEST_INPUT) {
		AbstractPurpleRequest *r = (AbstractPurpleRequest *) ui_handle;
		if (r->requestType() == CALLER_ADHOC) {
			AdhocCommandHandler * repeater = (AdhocCommandHandler *) r;
			std::string from = repeater->from();
			GlooxMessageHandler::instance()->adhoc()->unregisterSession(from);
			delete repeater;
		}
		else if (r->requestType() == CALLER_SEARCH) {
			SearchRepeater * repeater = (SearchRepeater *) r;
			std::string from = repeater->from();
			GlooxMessageHandler::instance()->searchHandler()->unregisterSession(from);
			delete repeater;
		}
	}
}

/*
 * Called when somebody from legacy network wants to send file to us.
 */
static void newXfer(PurpleXfer *xfer){
	Log().Get("purple filetransfer") << "new file transfer request from legacy network";
	GlooxMessageHandler::instance()->purpleFileReceiveRequest(xfer);
}

/*
 * Called when file from legacy network is completely received.
 */
static void XferComplete(PurpleXfer *xfer){
	Log().Get("purple filetransfer") << "filetransfer complete";
	GlooxMessageHandler::instance()->purpleFileReceiveComplete(xfer);
}

/*
 * Called on every buddy list change. We call buddyChanged from here... :)
 */
static void buddyListUpdate(PurpleBuddyList *blist, PurpleBlistNode *node){
	if (node != NULL) {
		if (node->type == PURPLE_BLIST_BUDDY_NODE) {
			PurpleBuddy* buddy = (PurpleBuddy*)node;
			GlooxMessageHandler::instance()->purpleBuddyChanged(buddy);
		}
	}
}

/*
 * Called when somebody from legacy network wants to authorize some jabber user.
 * We can return some object which will be connected with this request all the time...
 */
static void * accountRequestAuth(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data){
	Log().Get("purple") << "new AUTHORIZE REQUEST";
	return GlooxMessageHandler::instance()->purpleAuthorizeReceived(account,remote_user,id,alias,message,on_list,authorize_cb,deny_cb,user_data);
}

/*
 * Called when account is disconnecting and all requests will be closed and unreachable.
 */
static void accountRequestClose(void *data){
	Log().Get("purple") << "AUTHORIZE REQUEST CLOSE";
	GlooxMessageHandler::instance()->purpleAuthorizeClose(data);
}

/*
 * Called when user info (VCard in Jabber world) arrived from legacy network.
 */
static void * notify_user_info(PurpleConnection *gc, const char *who, PurpleNotifyUserInfo *user_info)
{
	std::string name(who);
	std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	GlooxMessageHandler::instance()->vcard()->userInfoArrived(gc, name, user_info);
	return NULL;
}

static void * notifyEmail(PurpleConnection *gc, const char *subject, const char *from, const char *to, const char *url)
{
	GlooxMessageHandler::instance()->notifyEmail(gc, subject, from, to, url);
	return NULL;
}

static void * notifyMessage(PurpleNotifyMsgType type, const char *title, const char *primary, const char *secondary) {
	// TODO: We have to patch libpurple to be able to identify from which account the message came from...
	// without this the function is quite useles...
	
// 	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
// 	if (user && !user->adhocData().id.empty()) {
// 		AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primary ? std::string(primary):std::string(), secondary ? std::string(secondary):std::string(), default_action, user_data, action_count, actions);
// 		GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
// 		AdhocData data;
// 		data.id="";
// 		user->setAdhocData(data);
// 		return repeater;
// 	}
	return NULL;
}

static void XferCreated(PurpleXfer *xfer) {
	std::string remote_user(purple_xfer_get_remote_user(xfer));
	Log().Get("xfercreated") << "get user " << remote_user;
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (!user) return;
	
	FiletransferRepeater *repeater = user->getFiletransfer(remote_user);
	Log().Get(user->jid()) << "get filetransferRepeater" << remote_user;
	if (!repeater) return;
	Log().Get(user->jid()) << "registerXfer";
	repeater->registerXfer(xfer);

}

static void fileSendStart(PurpleXfer *xfer) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->fileSendStart();
	Log().Get("filesend") << "fileSendStart()";
}


static void buddyListAddBuddy(PurpleAccount *account, const char *username, const char *group, const char *alias){
	std::cout << "BUDDY LIST ADD BUDDY REQUEST\n";
}

/*
 * Ops....
 */

static PurpleNotifyUiOps notifyUiOps =
{
		notifyMessage,
		notifyEmail,
		NULL,
		NULL,
		notifySearchResults,
		NULL,
		notify_user_info,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
};

static PurpleAccountUiOps accountUiOps =
{
	NULL,
	NULL,
	NULL,
	accountRequestAuth,
	accountRequestClose,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurpleBlistUiOps blistUiOps =
{
	NULL,
	NULL,
	NULL,
	buddyListUpdate,
	NULL,
	NULL,
	NULL,
	buddyListAddBuddy,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurpleRequestUiOps requestUiOps =
{
	requestInput,
	NULL,
	requestAction,
	requestFields,
	NULL,
	requestClose,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurpleXferUiOps xferUiOps =
{
	XferCreated,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurpleConnectionUiOps conn_ui_ops =
{
	NULL,
	NULL,
	NULL,//connection_disconnected,
	NULL,
	NULL,
	NULL,
	NULL,
	connection_report_disconnect,
	NULL,
	NULL,
	NULL
};

static PurpleConversationUiOps conversation_ui_ops =
{
	NULL,//pidgin_conv_new,
	NULL,//pidgin_conv_destroy,              /* destroy_conversation */
	conv_write_chat,                              /* write_chat           */
	conv_write_im,             /* write_im             */
	conv_write_conv,           /* write_conv           */
	conv_chat_add_users,       /* chat_add_users       */
	conv_chat_rename_user,     /* chat_rename_user     */
	conv_chat_remove_users,    /* chat_remove_users    */
	NULL,//pidgin_conv_chat_update_user,     /* chat_update_user     */
	NULL,//pidgin_conv_present_conversation, /* present              */
	NULL,//pidgin_conv_has_focus,            /* has_focus            */
	NULL,//pidgin_conv_custom_smiley_add,    /* custom_smiley_add    */
	NULL,//pidgin_conv_custom_smiley_write,  /* custom_smiley_write  */
	NULL,//pidgin_conv_custom_smiley_close,  /* custom_smiley_close  */
	NULL,//pidgin_conv_send_confirm,         /* send_confirm         */
	NULL,
	NULL,
	NULL,
	NULL
};


/***** Core Ui Ops *****/
static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops(&blistUiOps);
	purple_accounts_set_ui_ops(&accountUiOps);
	purple_notify_set_ui_ops(&notifyUiOps);
	purple_request_set_ui_ops(&requestUiOps);
	purple_xfers_set_ui_ops(&xferUiOps);
	purple_connections_set_ui_ops(&conn_ui_ops);
	purple_conversations_set_ui_ops(&conversation_ui_ops);
}

static PurpleCoreUiOps coreUiOps =
{
	NULL,
	NULL,
	transport_core_ui_init,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/*
 * Called when file is received from legacy network and we can send it to jabber user... :)
 */
static gboolean sendFileToJabber(gpointer data){
	fileTransferData *d = (fileTransferData*) data;
	std::cout << "SENDING FILE 1\n";
	if (d){
		std::cout << "SENDING FILE\n";
		GlooxMessageHandler::instance()->ftManager->sendFile(d->to,d->from,d->name,d->filename);
	}
	return FALSE;
}

static gboolean connectUser(gpointer data){
	std::string name((char*)data);
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
	if (user && user->readyForConnect() && !user->isConnected()){
		user->connect();
	}
	g_free(data);
	return FALSE;
}

static gboolean reconnect(gpointer data){
	std::string name((char*)data);
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
	if (user){
		user->connect();
	}
	g_free(data);
	return FALSE;
}

/*
 * Checking new connections for our gloox proxy...
 * TODO: rename me!
 */
static gboolean iter3(gpointer data){
// 	g_main_context_iteration(g_main_context_default(), FALSE);
	
	if (GlooxMessageHandler::instance()->ftServer->recv(1)==ConnNoError);
		return TRUE;
	std::cout << "ERROR!!!!\n";
	return FALSE;

}

static gboolean transportReconnect(gpointer data) {
	GlooxMessageHandler::instance()->transportConnect();
	return FALSE;
}

/*
 * Called by notifier when new data can be received from socket
 * TODO: rename me!
 */
static gboolean iter(GIOChannel *source,GIOCondition condition,gpointer data){
	GlooxMessageHandler::instance()->j->recv(1000);
	return TRUE;
}

GlooxMessageHandler::GlooxMessageHandler(const std::string &config) : MessageHandler(),ConnectionListener(),PresenceHandler(),SubscriptionHandler() {
	m_pInstance = this;
	m_firstConnection = true;
	lastIP=0;
	capsCache["_default"]=0;
	
	loadConfigFile(config);
	
	Log().Get("gloox") << "connecting to: " << m_configuration.server << " as " << m_configuration.jid << " with password " << m_configuration.password;
	j = new HiComponent("jabber:component:accept",m_configuration.server,m_configuration.jid,m_configuration.password,m_configuration.port);

	j->logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this );

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	signal(SIGCHLD, SIG_IGN);

	m_sql = new SQLClass(this);
	m_userManager = new UserManager(this);
	m_searchHandler = NULL;

	initPurple();
	loadProtocol();

	m_discoHandler=new GlooxDiscoHandler(this);
//  	j->removeIqHandler(XMLNS_DISCO_INFO);
	
 	m_discoInfoHandler=new GlooxDiscoInfoHandler(this);
 	j->registerIqHandler(m_discoInfoHandler,ExtDiscoInfo);
	
	m_adhoc = new GlooxAdhocHandler(this);
	m_parser = new GlooxParser();
	
	ftManager = new FileTransferManager();
	ft = new SIProfileFT(j, ftManager );
	ftManager->setSIProfileFT(ft,this);
 	ftServer = new SOCKS5BytestreamServer(j->logInstance(), 8000,configuration().bindIPs[0]);
	if( ftServer->listen() != ConnNoError )
		printf( "port in use\n" );
	ft->addStreamHost( j->jid(), configuration().bindIPs[0], 8000 );
	ft->registerSOCKS5BytestreamServer( ftServer );
	g_timeout_add(10,&iter3,NULL);
// 	ft->addStreamHost(gloox::JID("proxy.jabbim.cz"), "88.86.102.51", 7777);
	
	
	j->registerMessageHandler( this );
	j->registerConnectionListener(this);
	gatewayHandler = new GlooxGatewayHandler(this);
	j->registerIqHandler(gatewayHandler,ExtGateway);
	m_reg = new GlooxRegisterHandler(this);
	j->registerIqHandler(m_reg,ExtRegistration);
	// PING is now implemented in Gloox
// 	m_xping = new GlooxXPingHandler(this);
// 	j->registerIqHandler(m_xping,"urn:xmpp:ping");
	m_stats = new GlooxStatsHandler(this);
	j->registerIqHandler(m_stats,ExtStats);
	m_vcard = new GlooxVCardHandler(this);
	j->registerIqHandler(m_vcard,ExtVCard);
	j->registerPresenceHandler(this);
	j->registerSubscriptionHandler(this);
	
	transportConnect();
	
 	g_main_loop_run(loop);
}

GlooxMessageHandler::~GlooxMessageHandler(){
	delete m_userManager;
	delete m_parser;
	delete j;
	j=NULL;
	purple_core_quit();
}

void GlooxMessageHandler::loadProtocol(){
	if (configuration().protocol == "icq")
		m_protocol = (AbstractProtocol*) new ICQProtocol(this);
	else if (configuration().protocol == "facebook")
		m_protocol = (AbstractProtocol*) new FacebookProtocol(this);
	else if (configuration().protocol == "gg")
		m_protocol = (AbstractProtocol*) new GGProtocol(this);
	else if (configuration().protocol == "msn")
		m_protocol = (AbstractProtocol*) new MSNProtocol(this);
	else if (configuration().protocol == "irc")
		m_protocol = (AbstractProtocol*) new IRCProtocol(this);
	else if (configuration().protocol == "xmpp")
		m_protocol = (AbstractProtocol*) new XMPPProtocol(this);
	else if (configuration().protocol == "myspace")
		m_protocol = (AbstractProtocol*) new MyspaceProtocol(this);
	else if (configuration().protocol == "qq")
		m_protocol = (AbstractProtocol*) new QQProtocol(this);
	else if (configuration().protocol == "simple")
		m_protocol = (AbstractProtocol*) new SimpleProtocol(this);
	else if (configuration().protocol == "aim")
		m_protocol = (AbstractProtocol*) new AIMProtocol(this);
	if (!m_protocol->userSearchAction().empty()) {
		m_searchHandler = new GlooxSearchHandler(this);
		j->registerIqHandler(m_searchHandler, ExtSearch);
	}
}

void GlooxMessageHandler::onSessionCreateError (SessionCreateError error){
	Log().Get("gloox") << "sessionCreateError";
}

void GlooxMessageHandler::purpleConnectionError(PurpleConnection *gc,PurpleConnectionError reason,const char *text){
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = userManager()->getUserByAccount(account);
	if (user!=NULL){
		Log().Get(user->jid()) << "Disconnected from legacy network because of error" << int(reason);
		if (text)
			Log().Get(user->jid()) << std::string(text);
		// fatal error => account will be disconnected, so we have to remove it
		if (reason!=0){
			if (text){
				Message s(Message::Chat, user->jid(), tr(user->getLang(), text));
				std::string from;
				s.setFrom(jid());
				j->send(s);
			}
// 			if (user->isConnected()==true){
// 				user->isConnected()=false;
// 			}
			m_userManager->removeUserTimer(user);
		}
		else{
			if (user->reconnectCount() > 0){
				if (text){
					Message s(Message::Chat, user->jid(), tr(user->getLang(),text));
					std::string from;
					s.setFrom(jid());
					j->send(s);
				}
// 				if (user->isConnected()==true){
// 					user->isConnected()=false;
// 				}
				m_userManager->removeUserTimer(user);
			}
			else{
				user->disconnected();
				g_timeout_add_seconds(5,&reconnect,g_strdup(user->jid().c_str()));
			}
		}
// 		}
// 		else {
// 			std::cout << "account is not disconnected\n";
// 		}
	}
}

void GlooxMessageHandler::purpleBuddyTyping(PurpleAccount *account, const char *who){
	User *user = userManager()->getUserByAccount(account);
	if (user!=NULL){
		user->purpleBuddyTyping((std::string)who);
	}
	else {
		Log().Get("purple") << "purpleBuddyTyping called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::purpleBuddyRemoved(PurpleBuddy *buddy) {
	if (buddy != NULL){
		PurpleAccount *a = purple_buddy_get_account(buddy);
		User *user = userManager()->getUserByAccount(a);
		if (user!=NULL)
			user->purpleBuddyRemoved(buddy);
	}
}

void GlooxMessageHandler::purpleBuddyTypingStopped(PurpleAccount *account, const char *who){
	User *user = userManager()->getUserByAccount(account);
	if (user!=NULL){
		user->purpleBuddyTypingStopped((std::string)who);
	}
	else{
		Log().Get("purple") << "purpleBuddyTypingStopped called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::signedOn(PurpleConnection *gc,gpointer unused){
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = userManager()->getUserByAccount(account);
	if (user!=NULL){
		Log().Get(user->jid()) << "logged in to legacy network";
		user->connected();
	}
}

void GlooxMessageHandler::purpleAuthorizeClose(void *data){
	authData *d = (authData*) data;
	User *user = userManager()->getUserByAccount(d->account);
	if (user != NULL) {
		Log().Get(user->jid()) << "purple wants to close authorizeRequest";
		if (user->hasAuthRequest(d->who)) {
			Log().Get(user->jid()) << "closing authorizeRequest";
			user->removeAuthRequest(d->who);
		}
	}
	else{
		Log().Get("purple") << "purpleAuthorizationClose called, but user does not exist!!!";
	}
}

void * GlooxMessageHandler::purpleAuthorizeReceived(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data){
	if (account==NULL)
		return NULL;
	User *user = userManager()->getUserByAccount(account);
	if (user!=NULL) {
		if (user->isConnected()) {
			user->purpleAuthorizeReceived(account,remote_user,id,alias,message,on_list,authorize_cb,deny_cb,user_data);
			authData *data = new authData;
			data->account = account;
			data->who.assign(remote_user);
			return data;
		}
		else {
			Log().Get(user->jid()) << "purpleAuthorizeReceived called for unconnected user...";
		}
	}
	else {
		Log().Get("purple") << "purpleAuthorizeReceived called, but user does not exist!!!";
	}
	return NULL;
// 	<presence type='subscribe'
//           from='CapuletNurse@aim.shakespeare.lit'
//           to='romeo@montague.lit'/>
}

/*
 * Load config file and parses it and save result to m_configuration
 */
void GlooxMessageHandler::loadConfigFile(const std::string &config) {
	GKeyFile *keyfile;
	int flags;
  	char **bind;
	int i;
	
	keyfile = g_key_file_new ();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
	
	if (!g_key_file_load_from_file (keyfile, config.c_str(), (GKeyFileFlags)flags, NULL)) {
		if (!g_key_file_load_from_file (keyfile, std::string("/etc/spectrum/" + config + ".cfg").c_str(), (GKeyFileFlags)flags, NULL))
		{
			Log().Get("gloox") << "Can't load config file!";
			Log().Get("gloox") << std::string("/etc/spectrum/" + config + ".cfg") << " or ./" << config;
			
			g_key_file_free(keyfile);
			exit(0);
			return;
		}
	}

	m_configuration.discoName = (std::string)g_key_file_get_string(keyfile, "service","name", NULL);
	m_configuration.protocol = (std::string)g_key_file_get_string(keyfile, "service","protocol", NULL);
	m_configuration.server = (std::string)g_key_file_get_string(keyfile, "service","server", NULL);
	m_configuration.password = (std::string)g_key_file_get_string(keyfile, "service","password", NULL);
	m_configuration.jid = (std::string)g_key_file_get_string(keyfile, "service","jid", NULL);
	m_configuration.port = (int)g_key_file_get_integer(keyfile, "service","port", NULL);
	m_configuration.filetransferCache = (std::string)g_key_file_get_string(keyfile, "service","filetransfer_cache", NULL);
	
	m_configuration.sqlType = (std::string)g_key_file_get_string(keyfile, "database","type", NULL);
	m_configuration.sqlHost = (std::string)g_key_file_get_string(keyfile, "database","host", NULL);
	m_configuration.sqlPassword = (std::string)g_key_file_get_string(keyfile, "database","password", NULL);
	m_configuration.sqlUser = (std::string)g_key_file_get_string(keyfile, "database","user", NULL);
	m_configuration.sqlDb = (std::string)g_key_file_get_string(keyfile, "database","database", NULL);
	m_configuration.sqlPrefix = (std::string)g_key_file_get_string(keyfile, "database","prefix", NULL);

	m_configuration.userDir = (std::string)g_key_file_get_string(keyfile, "purple","userdir", NULL);

	if(g_key_file_has_key(keyfile,"service","language",NULL))
		m_configuration.language=g_key_file_get_string(keyfile, "service","language", NULL);
	else
		m_configuration.language = "en";
	
	if(g_key_file_has_key(keyfile,"service","only_for_vip",NULL))
		m_configuration.onlyForVIP=g_key_file_get_boolean(keyfile, "service","only_for_vip", NULL);
	else
		m_configuration.onlyForVIP=false;

	if(g_key_file_has_key(keyfile,"service","vip_mode",NULL))
		m_configuration.VIPEnabled=g_key_file_get_boolean(keyfile, "service","vip_mode", NULL);
	else
		m_configuration.VIPEnabled=false;

	if(g_key_file_has_key(keyfile,"service","transport_features",NULL)) {
		bind = g_key_file_get_string_list (keyfile,"service","transport_features",NULL, NULL);
		m_configuration.transportFeatures = 0;
		for (i = 0; bind[i]; i++){
			std::string feature(bind[i]);
			if (feature == "avatars")
				m_configuration.transportFeatures = m_configuration.transportFeatures | TRANSPORT_FEATURE_AVATARS;
			else if (feature == "chatstate")
				m_configuration.transportFeatures = m_configuration.transportFeatures | TRANSPORT_FEATURE_TYPING_NOTIFY;
			else if (feature == "filetransfer")
				m_configuration.transportFeatures = m_configuration.transportFeatures | TRANSPORT_FEATURE_FILETRANSFER;
		}
		g_strfreev (bind);
	}
	else m_configuration.transportFeatures = 255;
	
	if(g_key_file_has_key(keyfile,"service","vip_features",NULL)) {
		bind = g_key_file_get_string_list (keyfile,"service","vip_features",NULL, NULL);
		m_configuration.VIPFeatures = 0;
		for (i = 0; bind[i]; i++){
			std::string feature(bind[i]);
			if (feature == "avatars")
				m_configuration.VIPFeatures = m_configuration.VIPFeatures | TRANSPORT_FEATURE_AVATARS;
			else if (feature == "chatstate")
				m_configuration.VIPFeatures = m_configuration.VIPFeatures | TRANSPORT_FEATURE_TYPING_NOTIFY;
			else if (feature == "filetransfer")
				m_configuration.VIPFeatures = m_configuration.VIPFeatures | TRANSPORT_FEATURE_FILETRANSFER;
		}
		g_strfreev (bind);
	}
	else m_configuration.VIPFeatures = 255;

	if(g_key_file_has_key(keyfile,"service","use_proxy",NULL))
		m_configuration.useProxy=g_key_file_get_boolean(keyfile, "service","use_proxy", NULL);
	else
		m_configuration.useProxy = false;

	if(g_key_file_has_key(keyfile,"purple","bind",NULL)) {
		bind = g_key_file_get_string_list (keyfile,"purple","bind",NULL, NULL);
		for (i = 0; bind[i]; i++){
			m_configuration.bindIPs[i] = (std::string)bind[i];
		}
		g_strfreev (bind);
	}
	
	if(g_key_file_has_key(keyfile,"service","allowed_servers",NULL)) {
		bind = g_key_file_get_string_list(keyfile, "service", "allowed_servers", NULL, NULL);
		for (i = 0; bind[i]; i++){
			m_configuration.allowedServers.push_back((std::string) bind[i]);
		}
		g_strfreev (bind);
	}

	g_key_file_free(keyfile);
}

void GlooxMessageHandler::purpleFileReceiveRequest(PurpleXfer *xfer) {
	std::string tempname(purple_xfer_get_filename(xfer));
	std::string remote_user(purple_xfer_get_remote_user(xfer));

    std::string filename;

    filename.resize(tempname.size());

    utf8::replace_invalid(tempname.begin(), tempname.end(), filename.begin(), '_');

    // replace invalid characters
    for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
        if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
            *it = '_';
        }
    }

	purple_xfer_request_accepted(xfer, std::string(configuration().filetransferCache+"/"+remote_user+"-"+j->getID()+"-"+filename).c_str());
	User *user = userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (user!=NULL){
		if(user->hasFeature(GLOOX_FEATURE_FILETRANSFER)){
			Message s(Message::Chat, user->jid(), tr(user->getLang(),_("User is sending you file '"))+filename+tr(user->getLang(),_("'. It will be resend to you right after we receive it.")));
			s.setFrom(remote_user+"@"+jid()+"/bot");
			j->send(s);
		}
		else{
			Message s(Message::Chat, user->jid(), tr(user->getLang(),_("User is sending you file '"))+filename+tr(user->getLang(),_("'. We will send you link to the file right when we receive it.")));
			s.setFrom(remote_user+"@"+jid()+"/bot");
			j->send(s);
		}
	}
}

void GlooxMessageHandler::purpleFileReceiveComplete(PurpleXfer *xfer) {
	std::string filename(purple_xfer_get_filename(xfer));
	std::string remote_user(purple_xfer_get_remote_user(xfer));
	std::string localname(purple_xfer_get_local_filename(xfer));
	std::string basename(g_path_get_basename(purple_xfer_get_local_filename(xfer)));
	User *user = userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (user!=NULL) {
		if (user->isConnected()) {
			Log().Get(user->jid()) << "Trying to send file " << filename;
			if(user->hasFeature(GLOOX_FEATURE_FILETRANSFER)) {
				if (user->hasTransportFeature(TRANSPORT_FEATURE_FILETRANSFER)) {
					Log().Get(user->jid()) << "Trying to send file got feature";
					fileTransferData *data = new fileTransferData;
					data->to=user->jid() + "/" + user->resource();
					data->from=remote_user+"@"+jid()+"/bot";
					data->filename=localname;
					data->name=filename;
					g_timeout_add_seconds(1,&sendFileToJabber,data);
					sql()->addDownload(basename,"1");
				}
				else {
					sql()->addDownload(basename,"0");
				}
				Message s(Message::Chat, user->jid(), tr(user->getLang(),_("File '"))+filename+tr(user->getLang(),_("' was received. You can download it here: ")) + "http://soumar.jabbim.cz/icq/" + basename +" .");
				s.setFrom(remote_user+"@"+jid()+"/bot");
				j->send(s);
			}
			else{
				if (user->isVIP()){
					sql()->addDownload(basename,"1");
				}
				else {
					sql()->addDownload(basename,"0");
				}
				Message s(Message::Chat, user->jid(), tr(user->getLang(),_("File '"))+filename+tr(user->getLang(),_("' was received. You can download it here: ")) + "http://soumar.jabbim.cz/icq/" + basename +" .");
				s.setFrom(remote_user+"@"+jid()+"/bot");
				j->send(s);
			}
		}
		else{
			Log().Get(user->jid()) << "purpleFileReceiveComplete called for unconnected user...";
		}
	}
	else
		Log().Get("purple") << "purpleFileReceiveComplete called, but user does not exist!!!";
}


void GlooxMessageHandler::purpleMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags){
	User *user = userManager()->getUserByAccount(account);
	if (user){
		if (user->isConnected()){
			user->purpleMessageReceived(account,name,msg,conv,flags);
		}
		else {
			Log().Get(user->jid()) << "purpleMessageReceived called for unconnected user...";
		}
	}
	else{
		Log().Get("purple") << "purpleMessageReceived called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::purpleConversationWriteIM(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime){
	if (who==NULL)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()){
			m_stats->messageFromLegacy();
			user->purpleConversationWriteIM(conv,who,message,flags,mtime);
		}
		else {
			Log().Get(user->jid()) << "purpleConversationWriteIM called for unconnected user...";
		}
	}
	else {
		Log().Get("purple") << "purpleConversationWriteIM called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::notifyEmail(PurpleConnection *gc,const char *subject, const char *from,const char *to, const char *url) {
	if (protocol()->notifyUsername().empty())
		return;
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = userManager()->getUserByAccount(account);
	if (user!=NULL) {
		if (purple_value_get_boolean(user->getSetting("enable_notify_email"))) {
			std::string text;
			if (subject)
				text+=std::string(subject) + " ";
			if (from)
				text+=std::string(from) + " ";
			if (to)
				text+=std::string(to) + " ";
			if (url)
				text+=std::string(url) + " ";
			Message s(Message::Chat, user->jid(), text);
			s.setFrom(protocol()->notifyUsername()+"@"+jid()+"/bot");
			j->send(s);
		}
	}
}

void GlooxMessageHandler::purpleConversationWriteChat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime){
	if (who==NULL)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()){
			m_stats->messageFromLegacy();
			user->purpleConversationWriteChat(conv,who,message,flags,mtime);
		}
		else {
			Log().Get(user->jid()) << "purpleConversationWriteIM called for unconnected user...";
		}
	}
	else {
		Log().Get("purple") << "purpleConversationWriteIM called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			user->purpleChatTopicChanged(conv, who, topic);
		}
	}
}

void GlooxMessageHandler::purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()){
			user->purpleChatAddUsers(conv, cbuddies, new_arrivals);
		}
	}
}

void GlooxMessageHandler::purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()){
			user->purpleChatRenameUser(conv, old_name, new_name, new_alias);
		}
	}
}

void GlooxMessageHandler::purpleChatRemoveUsers(PurpleConversation *conv, GList *users) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()){
			user->purpleChatRemoveUsers(conv, users);
		}
	}
}

void GlooxMessageHandler::purpleBuddyChanged(PurpleBuddy* buddy){
	if (buddy!=NULL){
		PurpleAccount *a=purple_buddy_get_account(buddy);
		User *user = userManager()->getUserByAccount(a);
		if (user!=NULL)
			user->purpleBuddyChanged(buddy);
	}
	
}

bool GlooxMessageHandler::hasCaps(const std::string &name){
	std::map<std::string,int> ::iterator iter = capsCache.begin();
	iter = capsCache.find(name);
	if(iter != capsCache.end())
		return true;
	return false;
}

void GlooxMessageHandler::handleSubscription(const Subscription &stanza) {
	// answer to subscibe
	if(stanza.subtype() == Subscription::Subscribe && stanza.to().username() == "") {
		Log().Get(stanza.from().full()) << "Subscribe presence received => sending subscribed";
		Tag *reply = new Tag("presence");
		reply->addAttribute( "to", stanza.from().bare() );
		reply->addAttribute( "from", stanza.to().bare() );
		reply->addAttribute( "type", "subscribed" );
		j->send( reply );
	}

	User *user;
	if (protocol()->isMUC(NULL, stanza.to().bare())) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		user = userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = userManager()->getUserByJID(stanza.from().bare());
	}
	if (user)
		user->receivedSubscription(stanza);
	
}

void GlooxMessageHandler::handlePresence(const Presence &stanza){
	if(stanza.subtype() == Presence::Error) {
		return;
	}
	// get entity capabilities
	Tag *c = NULL;
	Log().Get(stanza.from().full()) << "Presence received (" << stanza.subtype() << ") for: " << stanza.to().full();

	if (stanza.presence() != Presence::Unavailable && ((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || protocol()->isMUC(NULL, stanza.to().bare()))){
		Tag *stanzaTag = stanza.tag();
		if (!stanzaTag) return;
		Tag *c = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/caps");
		// presence has caps
		if (c!=NULL){
			// caps is not chached
			if (!hasCaps(c->findAttribute("ver"))){
				// ask for caps
				std::string id = j->getID();
				Log().Get(stanza.from().full()) << "asking for caps with ID: " << id;
				m_discoHandler->versions[m_discoHandler->version].version=c->findAttribute("ver");
				m_discoHandler->versions[m_discoHandler->version].jid=stanza.to().full();
				std::string node;
				node = c->findAttribute("node")+std::string("#")+c->findAttribute("ver");
				j->disco()->getDiscoInfo(stanza.from(),node,m_discoHandler,m_discoHandler->version,id);
				m_discoHandler->version++;
			}
			else {
				std::string id = j->getID();
				Log().Get(stanza.from().full()) << "asking for disco#info with ID: " << id;
				m_discoHandler->versions[m_discoHandler->version].version=stanza.from().full();
				m_discoHandler->versions[m_discoHandler->version].jid=stanza.to().full();
				j->disco()->getDiscoInfo(stanza.from(),"",m_discoHandler,m_discoHandler->version,id);
				m_discoHandler->version++;
			}
		}
		else {
			std::string id = j->getID();
			Log().Get(stanza.from().full()) << "asking for disco#info with ID: " << id;
			m_discoHandler->versions[m_discoHandler->version].version=stanza.from().full();
			m_discoHandler->versions[m_discoHandler->version].jid=stanza.to().full();
			j->disco()->getDiscoInfo(stanza.from(),"",m_discoHandler,m_discoHandler->version,id);
			m_discoHandler->version++;
		}
		delete stanzaTag;
	}
	User *user;
	if (protocol()->isMUC(NULL, stanza.to().bare())) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		user = userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = userManager()->getUserByJID(stanza.from().bare());
	}
	if (user==NULL){
		// we are not connected and probe arrived => answer with unavailable
		if (stanza.subtype() == Presence::Probe){
			Log().Get(stanza.from().full()) << "Answering to probe presence with unavailable presence";
			Tag *tag = new Tag("presence");
			tag->addAttribute("to",stanza.from().full());
			tag->addAttribute("from",stanza.to().bare()+"/bot");
			tag->addAttribute("type","unavailable");
			j->send(tag);
		}
		else if (((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || protocol()->isMUC(NULL, stanza.to().bare())) && stanza.presence() != Presence::Unavailable){
			UserRow res = sql()->getUserByJid(stanza.from().bare());
			if(res.id==-1 && !protocol()->tempAccountsAllowed()) {
				// presence from unregistered user
				Log().Get(stanza.from().full()) << "This user is not registered";
				return;
			}
			else {
				bool isVip = sql()->isVIP(stanza.from().bare());
				std::list<std::string> const &x = configuration().allowedServers;
				if (configuration().onlyForVIP && !isVip && std::find(x.begin(), x.end(), stanza.from().server()) == x.end()){
					Log().Get(stanza.from().full()) << "This user is not VIP, can't login...";
					return;
				}

				Log().Get(stanza.from().full()) << "Creating new User instance";
				if (protocol()->tempAccountsAllowed()) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					std::cout << "SERVER" << stanza.from().bare() + server << "\n";
					user = new User(this, stanza.from(), stanza.to().resource() + "@" + server, "", stanza.from().bare() + server);
				}
				else
					user = new User(this, stanza.from(), res.uin, res.password, stanza.from().bare());
				user->setFeatures(isVip ? configuration().VIPFeatures : configuration().transportFeatures);
				if (c!=NULL)
					if(hasCaps(c->findAttribute("ver")))
						user->setResource(stanza.from().resource(), stanza.priority(), c->findAttribute("ver"));

				std::map<int,std::string> ::iterator iter = configuration().bindIPs.begin();
				iter = configuration().bindIPs.find(lastIP);
				if(iter != configuration().bindIPs.end()){
					user->setBindIP(configuration().bindIPs[lastIP]);
					lastIP++;
				}
				else{
					lastIP=0;
					user->setBindIP(configuration().bindIPs[lastIP]);
				}
// 				user->init(this);
				m_userManager->addUser(user);
				user->receivedPresence(stanza);
				if (protocol()->isMUC(NULL, stanza.to().bare())) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					server = stanza.from().bare() + server;
					g_timeout_add_seconds(15,&connectUser,g_strdup(server.c_str()));
				}
				else
					g_timeout_add_seconds(15,&connectUser,g_strdup(stanza.from().bare().c_str()));
			}
		}
		if (stanza.presence() == Presence::Unavailable && stanza.to().username() == ""){
			Log().Get(stanza.from().full()) << "User is already logged out => sending unavailable presence";
			Tag *tag = new Tag("presence");
			tag->addAttribute( "to", stanza.from().bare() );
			tag->addAttribute( "type", "unavailable" );
			tag->addAttribute( "from", jid() );
			j->send( tag );
		}
	}
	else{
		user->receivedPresence(stanza);
	}
	if(stanza.to().username() == "" && user!=NULL){
		if(stanza.presence() == Presence::Unavailable && user->isConnected()==true && user->resources().empty()) {
			Log().Get(stanza.from().full()) << "Logging out";
			m_userManager->removeUser(user);
		}
		else if (stanza.presence() == Presence::Unavailable && user->isConnected()==false && int(time(NULL))>int(user->connectionStart())+10 && user->resources().empty()){
			Log().Get(stanza.from().full()) << "Logging out, but he's not connected...";
			m_userManager->removeUser(user);
		}
		else if (stanza.presence() == Presence::Unavailable && user->isConnected()==false){
			Log().Get(stanza.from().full()) << "Can't logout because we're connecting now...";
		}
	}
    
}

void GlooxMessageHandler::onConnect(){
	Log().Get("gloox") << "CONNECTED!";
	j->disco()->setIdentity("gateway",protocol()->gatewayIdentity(),configuration().discoName);
	j->disco()->setVersion(configuration().discoName,"0.1","");
	std::list<std::string> features = protocol()->transportFeatures();
	for(std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
		j->disco()->addFeature(*it);
	}
	if (m_firstConnection){
		new AutoConnectLoop(this);
		m_firstConnection = false;
	}
}

void GlooxMessageHandler::onDisconnect(ConnectionError e){
	Log().Get("gloox") << "!!!!!!!!!! DISCONNECTED FROM JABBER SERVER !!!!!!!!!!!";
	Log().Get("gloox") << j->streamError();
	Log().Get("gloox") << j->streamErrorText("default text");
	if (j->streamError()==0 || j->streamError()==24){
		Log().Get("gloox") << j->streamErrorText("trying to reconnect after 3 seconds");
		g_timeout_add_seconds(3,&transportReconnect, NULL);
	}
}

void GlooxMessageHandler::transportConnect() {
	j->connect(false);
	int mysock = dynamic_cast<ConnectionTCPClient*>( j->connectionImpl() )->socket();
	if (mysock > 0) {
		connectIO = g_io_channel_unix_new(mysock);
		g_io_add_watch(connectIO,(GIOCondition) READ_COND,&iter,NULL);
	}
}

bool GlooxMessageHandler::onTLSConnect(const CertInfo & info){
	return false;
}

void GlooxMessageHandler::handleMessage (const Message &msg, MessageSession *session) {
	if (msg.from().bare() == msg.to().bare())
		return;
	if (msg.subtype() == Message::Error || msg.subtype() == Message::Invalid)
		return;
	User *user;
	if (protocol()->isMUC(NULL, msg.to().bare())) {
		std::string server = msg.to().username().substr(msg.to().username().find("%") + 1, msg.to().username().length() - msg.to().username().find("%"));
		user = userManager()->getUserByJID(msg.from().bare() + server);
	}
	else {
		user = userManager()->getUserByJID(msg.from().bare());
	}
	if (user!=NULL){
		if (user->isConnected()){
			Tag *msgTag = msg.tag();
			if (!msgTag) return;
			Tag *chatstates = msgTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/chatstates");
			if (chatstates!=NULL){
				std::string username = msg.to().username();
				std::for_each( username.begin(), username.end(), replaceJidCharacters() );
				user->receivedChatState(username,chatstates->name());
			}
			if(msgTag->findChild("body")!=NULL) {
				m_stats->messageFromJabber();
				user->receivedMessage(msg);
			}
			else {
				// handle activity; TODO: move me to another function or better file
				Tag *event = msgTag->findChild("event");
				if (!event) return;
				Tag *items = event->findChildWithAttrib("node","http://jabber.org/protocol/activity");
				if (!items) return;
				Tag *item = items->findChild("item");
				if (!item) return;
				Tag *activity = item->findChild("activity");
				if (!activity) return;
				Tag *text = item->findChild("text");
				if (!text) return;
				std::string message(activity->cdata());
				if (message.empty()) return;

				user->actionData = message;

				if (purple_account_get_connection(user->account())) {
					PurpleConnection *gc = purple_account_get_connection(user->account());
					PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
					if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
						PurplePluginAction *action = NULL;
						GList *actions, *l;

						actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

						for (l = actions; l != NULL; l = l->next) {
							if (l->data) {
								action = (PurplePluginAction *) l->data;
								action->plugin = plugin;
								action->context = gc;
								if ((std::string) action->label == "Set Facebook status...") {
									action->callback(action);
								}
								purple_plugin_action_free(action);
							}
						}
					}
				}
			}
			delete msgTag;
		}
		else{
			Log().Get(msg.from().bare()) << "New message received, but we're not connected yet";
		}
	}
	else {
		Message s(Message::Chat, msg.from().full(), tr(configuration().language.c_str(),_("This message couldn't be sent, because you are not connected to legacy network. You will be automatically reconnected soon.")));
		s.setFrom(msg.to().full());
		j->send(s);
		Tag *stanza = new Tag("presence");
		stanza->addAttribute( "to", msg.from().bare());
		stanza->addAttribute( "type", "probe");
		stanza->addAttribute( "from", jid());
		j->send(stanza);
	}

}

bool GlooxMessageHandler::initPurple(){
	bool ret;
	GList *iter;
	int i;
	GList *names = NULL;

	purple_util_set_user_dir(configuration().userDir.c_str());


	purple_debug_set_enabled(true);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

	ret = purple_core_init(HIICQ_UI);
	if (ret) {
		static int conversation_handle;
		static int conn_handle;
		static int xfer_handle;
		static int blist_handle;
		iter = purple_plugins_get_protocols();
		for (i = 0; iter; iter = iter->next) {
			PurplePlugin *plugin = (PurplePlugin *)iter->data;
			PurplePluginInfo *info = (PurplePluginInfo *)plugin->info;
			if (info && info->name) {
				printf("\t%d: %s\n", i++, info->name);
				names = g_list_append(names, info->id);
			}
		}
		
		purple_set_blist(purple_blist_new());
		purple_blist_load();
		
		purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &conversation_handle, PURPLE_CALLBACK(newMessageReceived), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-send-start", &xfer_handle, PURPLE_CALLBACK(fileSendStart), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-recv-request", &xfer_handle, PURPLE_CALLBACK(newXfer), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-recv-complete", &xfer_handle, PURPLE_CALLBACK(XferComplete), NULL);
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);				
		purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);
	
		purple_commands_init();

	}
	return ret;
}


GlooxMessageHandler* GlooxMessageHandler::m_pInstance = NULL;

int main( int argc, char* argv[] ) {
	if (argc != 2)
		std::cout << "Usage: " << std::string(argv[0]) << " config_file_name or profile name\n";
	else {
		std::string config(argv[1]);
		GlooxMessageHandler t(config);
	}
}

