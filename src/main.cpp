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
#include <sys/stat.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include "utf8.h"
#include "log.h"
#include "geventloop.h"
#include "accountcollector.h"
#include "autoconnectloop.h"
#include "dnsresolver.h"
#include "usermanager.h"
#include "adhoc/adhochandler.h"
#include "adhoc/adhocrepeater.h"
#ifndef WIN32
#include "configinterface.h"
#endif
#include "searchhandler.h"
#include "searchrepeater.h"
#include "filetransferrepeater.h"
#include "registerhandler.h"
#include "spectrumdiscohandler.h"
#include "statshandler.h"
#include "vcardhandler.h"
#include "gatewayhandler.h"
#include "capabilityhandler.h"
#include "configfile.h"
#include "spectrum_util.h"
#include "sql.h"
#include "user.h"
#include "protocolmanager.h"
#include "filetransfermanager.h"
#include "localization.h"
#include "spectrumbuddy.h"
#include "spectrumnodehandler.h"
#include "transport.h"

#include "parser.h"
#include "commands.h"
#include "protocols/abstractprotocol.h"
#include "cmds.h"

#include "gloox/adhoc.h"
#include "gloox/error.h"
#include <gloox/tlsbase.h>
#include <gloox/compressionbase.h>
#include <gloox/mucroom.h>
#include <gloox/sha.h>
#include <gloox/vcardupdate.h>
#include <gloox/base64.h>
#include <gloox/chatstate.h>

#ifdef WITH_LIBEVENT
#include <event.h>
#endif

static gboolean nodaemon = FALSE;
static gchar *logfile = NULL;
static gchar *lock_file = NULL;
static gboolean ver = FALSE;
static gboolean upgrade_db = FALSE;
static gboolean check_db_version = FALSE;
static gboolean list_purple_settings = FALSE;

static GOptionEntry options_entries[] = {
	{ "nodaemon", 'n', 0, G_OPTION_ARG_NONE, &nodaemon, "Disable background daemon mode", NULL },
	{ "logfile", 'l', 0, G_OPTION_ARG_STRING, &logfile, "Set file to log", NULL },
	{ "pidfile", 'p', 0, G_OPTION_ARG_STRING, &lock_file, "File where to write transport PID", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Shows Spectrum version", NULL },
	{ "list-purple-settings", 's', 0, G_OPTION_ARG_NONE, &list_purple_settings, "Lists purple settings which can be used in config file", NULL },
	{ "upgrade-db", 'u', 0, G_OPTION_ARG_NONE, &upgrade_db, "Upgrades Spectrum database", NULL },
	{ "check-db-version", 'c', 0, G_OPTION_ARG_NONE, &check_db_version, "Checks Spectrum database version", NULL },
	{ NULL, 0, 0, G_OPTION_ARG_NONE, NULL, "", NULL }
};

struct DummyHandle {
	void *ptr;
};

static void daemonize(const char *cwd) {
#ifndef WIN32
	pid_t pid, sid;
	FILE* lock_file_f;
	char process_pid[20];

	/* already a daemon */
	if ( getppid() == 1 ) return;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(1);
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		exit(0);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		exit(1);
	}

	/* Change the current working directory.  This prevents the current
		directory from being locked; hence not being able to remove it. */
	if ((chdir(cwd)) < 0) {
		exit(1);
	}
	
    /* write our pid into it & close the file. */
	lock_file_f = fopen(lock_file, "w+");
	if (lock_file_f == NULL) {
		std::cout << "EE cannot create lock file " << lock_file << ". Exiting\n";
		exit(1);
    }
	sprintf(process_pid,"%d\n",getpid());
	if (fwrite(process_pid,1,strlen(process_pid),lock_file_f) < strlen(process_pid)) {
		std::cout << "EE cannot write to lock file " << lock_file << ". Exiting\n";
		exit(1);
	}
	fclose(lock_file_f);
	
	if (freopen( "/dev/null", "r", stdin) == NULL) {
		std::cout << "EE cannot open /dev/null. Exiting\n";
		exit(1);
	}
#endif
}


namespace gloox {
	class HiComponent : public Component {
		public:
			HiComponent(const std::string & ns, const std::string & server, const std::string & component, const std::string & password, int port = 5347) : Component(ns, server, component, password, port) {};
			virtual ~HiComponent() {};
	};
}

/*
 * New message from legacy network received (we can create conversation here)
 */
static void newMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags) {
	GlooxMessageHandler::instance()->purpleMessageReceived(account, name, msg, conv, flags);
}

/*
 * Called when libpurple wants to write some message to Chat
 */
static void conv_write_im(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	GlooxMessageHandler::instance()->purpleConversationWriteIM(conv, who, message, flags, mtime);
}

/*
 * Called when libpurple wants to write some message to Groupchat
 */
static void conv_write_chat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	GlooxMessageHandler::instance()->purpleConversationWriteChat(conv, who, message, flags, mtime);
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

static void conv_chat_update_user(PurpleConversation *conv, const char *user) {
	PurpleConvChatBuddy *cb = purple_conv_chat_cb_find(PURPLE_CONV_CHAT(conv), user);
	if (!cb)
		return;
	GList *cbuddies = NULL;
	cbuddies = g_list_prepend(cbuddies, cb);
	conv_chat_add_users(conv, cbuddies, 0);
}

/*
 * Called when user is renamed
 */
static void conv_chat_rename_user(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	GlooxMessageHandler::instance()->purpleChatRenameUser(conv, old_name, new_name, new_alias);
}

static void conv_destroy(PurpleConversation *conv) {
	GlooxMessageHandler::instance()->purpleConversationDestroyed(conv);
}

/*
 * Called when users are removed from chat
 */
static void conv_chat_remove_users(PurpleConversation *conv, GList *users) {
	GlooxMessageHandler::instance()->purpleChatRemoveUsers(conv, users);
}


/*
 * Called when somebody from legacy network start typing
 */
static void buddyTyping(PurpleAccount *account, const char *who, gpointer null) {
	GlooxMessageHandler::instance()->purpleBuddyTyping(account, who);
}

/*
 * Called when somebody from legacy network paused typing.
 */
static void buddyTyped(PurpleAccount *account, const char *who, gpointer null) {
	GlooxMessageHandler::instance()->purpleBuddyTypingPaused(account, who);
}

/*
 * Called when somebody from legacy network stops typing
 */
static void buddyTypingStopped(PurpleAccount *account, const char *who, gpointer null){
	GlooxMessageHandler::instance()->purpleBuddyTypingStopped(account, who);
}

/*
 * Called when PurpleBuddy is removed
 */
static void buddyRemoved(PurpleBuddy *buddy, gpointer null) {
	GlooxMessageHandler::instance()->purpleBuddyRemoved(buddy);
}

static void buddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status) {
	GlooxMessageHandler::instance()->purpleBuddyStatusChanged(buddy, status, old_status);
}

static void buddySignedOn(PurpleBuddy *buddy) {
	GlooxMessageHandler::instance()->purpleBuddySignedOn(buddy);
}

static void buddySignedOff(PurpleBuddy *buddy) {
	GlooxMessageHandler::instance()->purpleBuddySignedOff(buddy);
}

static void NodeRemoved(PurpleBlistNode *node, void *data) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(a);
	if (user != NULL) {
		user->handleBuddyRemoved(buddy);
	}
	if (buddy->node.ui_data) {
		SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
		Log("PurpleBuddy", "Deleting data for " << s_buddy->getName());
		delete s_buddy;
		buddy->node.ui_data = NULL;
	}
}

/*
 * Called when purple disconnects from legacy network.
 */
static void connection_report_disconnect(PurpleConnection *gc,PurpleConnectionError reason,const char *text){
	GlooxMessageHandler::instance()->purpleConnectionError(gc, reason, text);
}

/*
 * Called when purple wants to some input from transport.
 */
static void * requestInput(const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb,PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	std::string t(title ? title : "NULL");
	std::string s(secondary ? secondary : "NULL");
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	if (!user) {
		Log("requestInput", "WARNING: purple_request_input not handled. No user for account =" << purple_account_get_username(account));
		AbstractPurpleRequest *handle = new AbstractPurpleRequest;
		handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
		return handle;
	}

	if (primary) {
		std::string primaryString(primary);
		Log(user->jid(), "purple_request_input created:" << primaryString);
		// Check if there is some adhocData. If it's there, this request will be handled by some handler registered to this data.
		if (!user->adhocData().id.empty()) {
			if (user->adhocData().callerType == CALLER_ADHOC) {
				AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title) : std::string(), primaryString, secondary ? std::string(secondary) : std::string(), default_value ? std::string(default_value) : std::string(), multiline, masked, ok_cb, cancel_cb, user_data);
				GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
				AdhocData data;
				data.id = "";
				user->setAdhocData(data);
				return repeater;
			}
			else if (user->adhocData().callerType == CALLER_SEARCH) {
				SearchRepeater *repeater = new SearchRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title) : std::string(), primaryString, secondary ? std::string(secondary) : std::string(), default_value ? std::string(default_value) : std::string(), multiline, masked, ok_cb, cancel_cb, user_data);
				GlooxMessageHandler::instance()->searchHandler()->registerSession(user->adhocData().from, repeater);
// 				AdhocData data;
// 				data.id="";
// 				user->setAdhocData(data);
				return repeater;
			}
		}
		// emit protocol signal
		AbstractPurpleRequest *handle = new AbstractPurpleRequest;
		handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
		bool handled = GlooxMessageHandler::instance()->protocol()->onPurpleRequestInput(handle, user, title, primary, secondary, default_value, multiline, masked, hint, ok_text, ok_cb, cancel_text, cancel_cb, account, who, conv, user_data);
		if (handled) {
			return handle;
		}
		else {
			delete handle;
		}
	}
	Log(user->jid(), "WARNING: purple_request_input not handled. primary == NULL, title ==" << t << ", secondary ==" << s);
	AbstractPurpleRequest *handle = new AbstractPurpleRequest;
	handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
	return handle;
}

static void * notifySearchResults(PurpleConnection *gc, const char *title, const char *primary, const char *secondary, PurpleNotifySearchResults *results, gpointer user_data) {
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(purple_connection_get_account(gc));
	if (!user) return NULL;
	if (!user->adhocData().id.empty()) {
		if (user->adhocData().callerType == CALLER_SEARCH) {
			GlooxMessageHandler::instance()->searchHandler()->searchResultsArrived(user->adhocData().from, results);
			AdhocData data;
			data.id = "";
			user->setAdhocData(data);
		}
	}
	AbstractPurpleRequest *handle = new AbstractPurpleRequest;
	handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
	return handle;
}

static void *notifyUri(const char *uri) {
	if (Transport::instance()->protocol()->onNotifyUri(uri) == false)
		Log("notifyUri", "WARNING:notifyUri not handled. " << uri);
	AbstractPurpleRequest *handle = new AbstractPurpleRequest;
	handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
	return handle;
}

static void *requestFields(const char *title, const char *primary, const char *secondary, PurpleRequestFields *fields, const char *ok_text, GCallback ok_cb, const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who, PurpleConversation *conv, void *user_data) {
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	if (user && !user->adhocData().id.empty()) {
		if (user->adhocData().callerType == CALLER_ADHOC) {
			AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title) : std::string(), primary ? std::string(primary) : std::string(), secondary ? std::string(secondary) : std::string(), fields, ok_cb, cancel_cb, user_data);
			GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
			AdhocData data;
			data.id = "";
			user->setAdhocData(data);
			return repeater;
		}
		else if (user->adhocData().callerType == CALLER_SEARCH) {
			SearchRepeater *repeater = new SearchRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primary ? std::string(primary) : std::string(), secondary ? std::string(secondary) : std::string(), fields, ok_cb, cancel_cb, user_data);
			GlooxMessageHandler::instance()->searchHandler()->registerSession(user->adhocData().from, repeater);
			return repeater;
		}
	}
	std::string t(title ? title : "NULL");
	std::string p(primary ? primary : "NULL");
	std::string s(secondary ? secondary : "NULL");
	Log("requestFields", "WARNING:requestFields not handled. " << (user ? user->jid() : "NULL") << " " << t << " " << p << " " << s);
	AbstractPurpleRequest *handle = new AbstractPurpleRequest;
	handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
	return handle;
}

static void * requestAction(const char *title, const char *primary,const char *secondary, int default_action,PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data,size_t action_count, va_list actions){
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	bool handled = false;
	
	std::string t(title ? title : "NULL");
	if (t == "SSL Certificate Verification") {
		Log("purple", "accepting SSL certificate");
		va_arg(actions, char *);
		((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
		handled = true;
	}
	else if (!user || !account) {
		std::string t(title ? title : "NULL");
		std::string p(primary ? primary : "NULL");
		std::string s(secondary ? secondary : "NULL");
		Log("requestAction", "WARNING: purple_request_action not handled. No user for account =" << (account ? purple_account_get_username(account) : "NULL") << " " << "title =" << t << ", primary =" << p << ", secondary =" << s);
	}
	else {
		if (!user->adhocData().id.empty()) {
			AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title) : std::string(), primary ? std::string(primary) : std::string(), secondary ? std::string(secondary) : std::string(), default_action, user_data, action_count, actions);
			GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
			AdhocData data;
			data.id = "";
			user->setAdhocData(data);
			return repeater;
		}
		else if (title) {
			std::string headerString(title);
			Log("purple", "header string: " << headerString);
			if (headerString == "SSL Certificate Verification") {
				va_arg(actions, char *);
				((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
				handled = true;
			}
		}
		if (!handled) {
			std::string t(title ? title : "NULL");
			std::string p(primary ? primary : "NULL");
			std::string s(secondary ? secondary : "NULL");
			Log(user->jid(), "WARNING: purple_request_action not handled. No handler for title =" << t << ", primary =" << p << ", secondary =" << s);
		}
	}
	AbstractPurpleRequest *handle = new AbstractPurpleRequest;
	handle->setRequestType((AdhocDataCallerType) CALLER_DUMMY);
	return handle;
}

static void requestClose(PurpleRequestType type, void *ui_handle) {
	if (!ui_handle)
		return;
	AbstractPurpleRequest *r = (AbstractPurpleRequest *) ui_handle;
	if (type == PURPLE_REQUEST_INPUT) {
		if (r->requestType() == CALLER_ADHOC) {
			AdhocCommandHandler * repeater = (AdhocCommandHandler *) r;
			std::string from = repeater->getInitiator();
			GlooxMessageHandler::instance()->adhoc()->unregisterSession(from);
			delete repeater;
			return;
		}
		else if (r->requestType() == CALLER_SEARCH) {
			SearchRepeater * repeater = (SearchRepeater *) r;
			std::string from = repeater->getInitiator();
			GlooxMessageHandler::instance()->searchHandler()->unregisterSession(from);
			delete repeater;
			return;
		}
	}
	else if (type == PURPLE_REQUEST_FIELDS || type == PURPLE_REQUEST_ACTION) {
		if (r->requestType() == CALLER_ADHOC) {
			AdhocCommandHandler * repeater = (AdhocCommandHandler *) r;
			std::string from = repeater->getInitiator();
			GlooxMessageHandler::instance()->adhoc()->unregisterSession(from);
			delete repeater;
			return;
		}
	}
	delete r;
}

/*
 * Called when somebody from legacy network wants to authorize some jabber user.
 * We can return some object which will be connected with this request all the time...
 */
static void * accountRequestAuth(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data) {
	return GlooxMessageHandler::instance()->purpleAuthorizeReceived(account, remote_user, id, alias, message, on_list, authorize_cb, deny_cb, user_data);
}

/*
 * Called when account is disconnecting and all requests will be closed and unreachable.
 */
static void accountRequestClose(void *data){
	GlooxMessageHandler::instance()->purpleAuthorizeClose(data);
}

/*
 * Called when user info (VCard in Jabber world) arrived from legacy network.
 */
static void * notify_user_info(PurpleConnection *gc, const char *who, PurpleNotifyUserInfo *user_info)
{
	std::string name(who);
// 	std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	GlooxMessageHandler::instance()->vcard()->userInfoArrived(gc, name, user_info);
	return NULL;
}

static void * notifyEmail(PurpleConnection *gc, const char *subject, const char *from, const char *to, const char *url)
{
	GlooxMessageHandler::instance()->notifyEmail(gc, subject, from, to, url);
	return NULL;
}

static void * notifyEmails(PurpleConnection *gc, size_t count, gboolean detailed, const char **subjects, const char **froms, const char **tos, const char **urls) {
	if (count == 0)
		return NULL;

	if (detailed) {
		for ( ; count; --count) {
			notifyEmail(gc, subjects ? *subjects : NULL, froms ? *froms : NULL, tos ? *tos : NULL, urls ? *urls : NULL);
			if (subjects)
				subjects++;
			if (froms)
				froms++;
			if (tos)
				tos++;
			if (urls)
				urls++;
		}
	}
	else {
		PurpleAccount *account = purple_connection_get_account(gc);
		User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
		std::string message = Poco::format(_("%s has %d new message."), std::string(*tos), (int) count );
		Message s(Message::Chat, user->jid(), tr(user->getLang(), message));
		s.setFrom(Transport::instance()->jid());
		Transport::instance()->send(s.tag());
	}
	
	return NULL;
}

static void * notifyMessage(PurpleNotifyMsgType type, const char *title, const char *primary, const char *secondary) {
	// TODO: We have to patch libpurple to be able to identify from which account the message came from...
	// without this the function is quite useles...

	// User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	// if (user && !user->adhocData().id.empty()) {
	// 	AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title):std::string(), primary ? std::string(primary):std::string(), secondary ? std::string(secondary):std::string(), default_action, user_data, action_count, actions);
	// 	GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
	// 	AdhocData data;
	// 	data.id="";
	// 	user->setAdhocData(data);
	// 	return repeater;
	// }
	Log("notifyMessage", std::string(title ? title : "") << " " << std::string(primary ? primary : "") << " " << std::string(secondary ? secondary : ""));
	return NULL;
}

static void buddyListSaveNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(a);
	if (!user) return;
	if (user->loadingBuddiesFromDB()) return;
	user->storeBuddy(buddy);

}

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	GlooxMessageHandler::instance()->purpleBuddyCreated(buddy);
}

static void buddyListRemoveNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(a);
	if (user != NULL) {
		if (user->loadingBuddiesFromDB()) return;
		long id = 0;
		if (buddy->node.ui_data) {
			SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
			if (s_buddy->getId() != -1) {
				id = s_buddy->getId();
			}
		}
		std::string name(purple_buddy_get_name(buddy));
		GlooxMessageHandler::instance()->sql()->removeBuddy(user->storageId(), name, id);
	}
}

static void buddyListSaveAccount(PurpleAccount *account) {
}

static void printDebug(PurpleDebugLevel level, const char *category, const char *arg_s) {
	std::string c("libpurple");

	if (category) {
		c.push_back('/');
		c.append(category);
	}

	LogMessage(Log_.fileStream(), false).Get(c) << arg_s;
}

/*
 * Ops....
 */
static PurpleDebugUiOps debugUiOps =
{
	printDebug,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


static PurpleNotifyUiOps notifyUiOps =
{
		notifyMessage,
		notifyEmail,
		notifyEmails,
		NULL,
		notifySearchResults,
		NULL,
		notify_user_info,
		notifyUri,
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
	buddyListNewNode,
	NULL,
	NULL, // buddyListUpdate,
	NULL, //NodeRemoved,
	NULL,
	NULL,
	NULL, // buddyListAddBuddy,
	NULL,
	NULL,
	buddyListSaveNode,
	buddyListRemoveNode,
	buddyListSaveAccount,
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
	conv_destroy,
	conv_write_chat,                              /* write_chat           */
	conv_write_im,             /* write_im             */
	conv_write_conv,           /* write_conv           */
	conv_chat_add_users,       /* chat_add_users       */
	conv_chat_rename_user,     /* chat_rename_user     */
	conv_chat_remove_users,    /* chat_remove_users    */
	conv_chat_update_user,     /* chat_update_user     */
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
static void
spectrum_glib_log_handler(const gchar *domain,
                          GLogLevelFlags flags,
                          const gchar *message,
                          gpointer user_data)
{
	const char *level;
	char *new_msg = NULL;
	char *new_domain = NULL;

	if ((flags & G_LOG_LEVEL_ERROR) == G_LOG_LEVEL_ERROR)
		level = "ERROR";
	else if ((flags & G_LOG_LEVEL_CRITICAL) == G_LOG_LEVEL_CRITICAL)
		level = "CRITICAL";
	else if ((flags & G_LOG_LEVEL_WARNING) == G_LOG_LEVEL_WARNING)
		level = "WARNING";
	else if ((flags & G_LOG_LEVEL_MESSAGE) == G_LOG_LEVEL_MESSAGE)
		level = "MESSAGE";
	else if ((flags & G_LOG_LEVEL_INFO) == G_LOG_LEVEL_INFO)
		level = "INFO";
	else if ((flags & G_LOG_LEVEL_DEBUG) == G_LOG_LEVEL_DEBUG)
		level = "DEBUG";
	else {
		Log("glib", "Unknown glib logging level in " << (guint)flags);
		level = "UNKNOWN"; /* This will never happen. */
	}

	if (message != NULL)
		new_msg = purple_utf8_try_convert(message);

	if (domain != NULL)
		new_domain = purple_utf8_try_convert(domain);

	if (new_msg != NULL) {
		std::string area("glib");
		area.push_back('/');
		area.append(level);

		std::string message(new_domain ? new_domain : "g_log");
		message.push_back(' ');
		message.append(new_msg);

		Log(area, message);
		g_free(new_msg);
	}

	g_free(new_domain);
}

static void
debug_init(void)
{
#define REGISTER_G_LOG_HANDLER(name) \
	g_log_set_handler((name), \
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL \
		                                  | G_LOG_FLAG_RECURSION), \
	                  spectrum_glib_log_handler, NULL)

	REGISTER_G_LOG_HANDLER(NULL);
	REGISTER_G_LOG_HANDLER("GLib");
	REGISTER_G_LOG_HANDLER("GModule");
	REGISTER_G_LOG_HANDLER("GLib-GObject");
	REGISTER_G_LOG_HANDLER("GThread");

#undef REGISTER_G_LOD_HANDLER
}

static void transport_core_ui_init(void)
{
	purple_blist_set_ui_ops(&blistUiOps);
	purple_accounts_set_ui_ops(&accountUiOps);
	purple_notify_set_ui_ops(&notifyUiOps);
	purple_request_set_ui_ops(&requestUiOps);
	purple_xfers_set_ui_ops(getXferUiOps());
	purple_connections_set_ui_ops(&conn_ui_ops);
	purple_conversations_set_ui_ops(&conversation_ui_ops);
#ifndef WIN32
	purple_dnsquery_set_ui_ops(getDNSUiOps());
#endif
}

static PurpleCoreUiOps coreUiOps =
{
	NULL,
	debug_init,
	transport_core_ui_init,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static gboolean getVCard(gpointer data) {
	std::string name((char*)data);
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
	if (user && user->isConnected()) {
		Transport::instance()->fetchVCard(name);
	}
	g_free(data);
	return FALSE;
}

/*
 * Reconnect user
 */
static gboolean reconnect(gpointer data) {
	std::string name((char*)data);
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
	if (user) {
		user->connect();
	}
	g_free(data);
	return FALSE;
}

static void listPurpleSettings() {
	std::cout << "You can use those variables in [purple] section in Spectrum config file.\n";
	PurplePlugin *plugin = purple_find_prpl(Transport::instance()->protocol()->protocol().c_str());
	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
	for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
		PurpleAccountOption *option = (PurpleAccountOption *) l->data;
		std::string key(purple_account_option_get_setting(option));
		std::string type;
		std::string def;

		switch (purple_account_option_get_type(option)) {
			case PURPLE_PREF_BOOLEAN:
				type = "boolean";
				def = stringOf(purple_account_option_get_default_bool(option));
				break;

			case PURPLE_PREF_INT:
				type = "integer";
				def = stringOf(purple_account_option_get_default_int(option));
				break;

			case PURPLE_PREF_STRING:
				type = "string";
				def = stringOf(purple_account_option_get_default_string(option));
				break;

			case PURPLE_PREF_STRING_LIST:
				type = "string list";
// 				def = StringOf(purple_account_option_get_default_string_list(option));
				break;

			default:
				continue;
		}
		std::cout << "Key: " << key << "\n";
		std::cout << "\tType: " << type << "\n";
		std::cout << "\tDescription: " << purple_account_option_get_text(option) << "\n";
		std::cout << "\tDefault value: " << def << "\n";
	}
}

GlooxMessageHandler::GlooxMessageHandler(const std::string &config){
}

GlooxMessageHandler::~GlooxMessageHandler(){
	delete m_userManager;
	purple_blist_uninit();
	purple_core_quit();
	if (m_loop) {
		g_main_loop_quit(m_loop);
		g_main_loop_unref(m_loop);
	}
#ifdef WITH_LIBEVENT
	else {
		event_loopexit(NULL);
	}
#endif
	
#ifndef WIN32
	if (m_configInterface)
		delete m_configInterface;
#endif
	if (m_parser)
		delete m_parser;
	if (m_sql)
		delete m_sql;
	if (ftManager)
		delete ftManager;
	if (m_reg)
		delete m_reg;
	if (gatewayHandler)
		delete gatewayHandler;
	if (m_stats)
		delete m_stats;
	if (m_adhoc)
		delete m_adhoc;
	if (m_vcardManager)
		delete m_vcardManager;
	// TODO: there are timers in commented classes, so wa have to stop them before purple_core_quit();
// 	if (ft)
// 		delete ft;
// 	if (ftServer)
// 		delete ftServer;
	if (m_protocol)
		delete m_protocol;
	if (m_searchHandler)
		delete m_searchHandler;
	delete m_transport;
// 	delete j;
}

static AbstractProtocol *loadProtocol(Configuration &cfg) {
	AbstractProtocol *m_protocol = NULL;
	for (GList *l = getSupportedProtocols(); l != NULL; l = l->next) {
		_spectrum_protocol *protocol = (_spectrum_protocol *) l->data;
		if (cfg.protocol == protocol->prpl_id) {
			m_protocol = protocol->create_protocol();
			break;
		}
	}
	if (m_protocol == NULL) {
		Log("loadProtocol", "Protocol \"" << cfg.protocol << "\" is not supported.");
		std::string protocols = "";
		for (GList *l = getSupportedProtocols(); l != NULL; l = l->next) {
			_spectrum_protocol *protocol = (_spectrum_protocol *) l->data;
			protocols += protocol->prpl_id;
			if (l->next)
				protocols += ", ";
		}
		Log("loadProtocol", "Protocol has to be one of: " << protocols << ".");
		return NULL;
	}

	if (!purple_find_prpl(m_protocol->protocol().c_str())) {
		Log("loadProtocol", "There is no libpurple plugin installed for protocol \"" << cfg.protocol << "\"");
		return NULL;
	}

	return m_protocol;
}

void GlooxMessageHandler::purpleConnectionError(PurpleConnection *gc,PurpleConnectionError reason,const char *text) {
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user != NULL) {
		Log(user->jid(), "Disconnected from legacy network because of error " << int(reason) << " " << std::string(text ? text : ""));
		if (text)
			Log(user->jid(), std::string(text));
		// fatal error => account will be disconnected, so we have to remove it
		if (reason != 0) {
// 			if (text){
// 				Message s(Message::Chat, user->jid(), tr(user->getLang(), text));
// 				std::string from;
// 				s.setFrom(jid());
// 				Transport::instance()->send(s);
// 			}
			Presence tag(Presence::Unavailable, user->jid(), tr(user->getLang(), _(text ? text : "")));
			tag.setFrom(Transport::instance()->jid());
			Transport::instance()->send(tag.tag());
			m_userManager->removeUserTimer(user);
		}
		else {
			if (user->reconnectCount() > 0) {
				Presence tag(Presence::Unavailable, user->jid(), tr(user->getLang(), _(text ? text : "")));
				tag.setFrom(Transport::instance()->jid());
				Transport::instance()->send(tag.tag());
				m_userManager->removeUserTimer(user);
			}
			else {
				user->disconnected();
				Log(user->jid(), "Reconnecting after 5 seconds");
				purple_timeout_add_seconds(5, &reconnect, g_strdup(user->jid().c_str()));
			}
		}
	}
}

void GlooxMessageHandler::purpleBuddyTyping(PurpleAccount *account, const char *who){
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user != NULL) {
		PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account);
		if (0 == conv) {
			return;
		}
		user->purpleBuddyTyping(who);
	}
	else {
		Log("purpleBuddyTyping", "WARNING: there is no User for account =" << purple_account_get_username(account));
	}
}

void GlooxMessageHandler::purpleBuddyRemoved(PurpleBuddy *buddy) {
// 	if (buddy != NULL) {
// 		PurpleAccount *a = purple_buddy_get_account(buddy);
// 		User *user = (User *) userManager()->getUserByAccount(a);
// 		if (user != NULL) {
// 			user->handleBuddyRemoved(buddy);
// 		}
// 	}
}

void GlooxMessageHandler::purpleBuddyCreated(PurpleBuddy *buddy) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) userManager()->getUserByAccount(a);
	if (user != NULL)
		user->handleBuddyCreated(buddy);
	else {
		buddy->node.ui_data = (void *) new SpectrumBuddy(-1, buddy);
		SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
		if (m_configuration.jid_escaping)
			s_buddy->setFlags(SPECTRUM_BUDDY_JID_ESCAPING);
	}
		
}

void GlooxMessageHandler::purpleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) userManager()->getUserByAccount(a);
	if (user != NULL)
		user->handleBuddyStatusChanged(buddy, status, old_status);
}

void GlooxMessageHandler::purpleBuddySignedOn(PurpleBuddy *buddy) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) userManager()->getUserByAccount(a);
	if (user != NULL)
		user->handleBuddySignedOn(buddy);
}

void GlooxMessageHandler::purpleBuddySignedOff(PurpleBuddy *buddy) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = (User *) userManager()->getUserByAccount(a);
	if (user != NULL)
		user->handleBuddySignedOff(buddy);
}

void GlooxMessageHandler::purpleBuddyTypingStopped(PurpleAccount *account, const char *who) {
	PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account);
	if (0 == conv) {
		return;
	}
  
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user != NULL) {
		user->purpleBuddyTypingStopped((std::string) who);
	}
	else {
		Log("purpleBuddyTypingStopped", "WARNING: there is no User for account =" << purple_account_get_username(account));
	}
}

void GlooxMessageHandler::purpleBuddyTypingPaused(PurpleAccount *account, const char *who) {
	PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account);
	if (0 == conv) {
		return;
	}
  
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user != NULL) {
		user->purpleBuddyTypingPaused((std::string) who);
	}
	else {
		Log("purpleBuddyTypingPaused", "WARNING: there is no User for account =" << purple_account_get_username(account));
	}
}

void GlooxMessageHandler::purpleAuthorizeClose(void *data) {
	authRequest *d = (authRequest *) data;
	if (!d)
		return;
	User *user = (User *) userManager()->getUserByAccount(d->account);
	// When User is disconnected because of an error, m_account->ui_data is NULL and
	// getUserByAccount can't find him, but some prpls removes authRequests in this
	// case. So we try to getUserByJID in that case.
	if (user == NULL) {
		user = (User *) userManager()->getUserByJID(d->mainJID);
	}
	if (user != NULL) {
		user->removeAuthRequest(d->who);
	}
	else {
		Log("purpleAuthorizeClose", "WARNING: there is no User for account =" << purple_account_get_username(d->account));
	}
	delete d;
}

void * GlooxMessageHandler::purpleAuthorizeReceived(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data){
	if (account==NULL)
		return NULL;
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user != NULL) {
		if (user->isConnected()) {
			return user->handleAuthorizationRequest(account, remote_user, id, alias, message, on_list, authorize_cb, deny_cb, user_data);
		}
		else {
			Log(user->jid(), "WARNING: purpleAuthorizeReceived for unconnected user from buddy = " << std::string(remote_user ? remote_user : "NULL"));
		}
	}
	else {
		Log("purpleAuthorizeReceived", "WARNING: there is no User for account =" << purple_account_get_username(account));
	}
	return NULL;
}

static Configuration loadConfigFile(const std::string &config) {
	ConfigFile cfg(config.empty() ? CONFIG().file : config);
	Configuration c = cfg.getConfiguration();

	if (Transport::instance() && CONFIG())
		cfg.loadPurpleAccountSettings(c);
	
	if (Transport::instance()) {
		if (!c && !CONFIG())
			return c;
	}
	else if (!c)
		return c;
	if (c) {
		if (Transport::instance() && CONFIG()) {
			c.username_mask = CONFIG().username_mask;
			c.hash = CONFIG().hash;
		}
	}

	if (logfile)
		c.logfile = std::string(logfile);
	
	if (!c.logfile.empty())
		Log_.setLogFile(c.logfile);

	if (!lock_file)
		lock_file = g_strdup(c.pid_f.c_str());
	
	return c;
}

void GlooxMessageHandler::purpleMessageReceived(PurpleAccount* account, char * name, char *msg, PurpleConversation *conv, PurpleMessageFlags flags) {
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			user->handlePurpleMessage(account,name,msg,conv,flags);
		}
		else {
			Log(user->jid(),"purpleMessageReceived called for unconnected user..."  << msg);
		}
	}
	else {
		Log("purple", "purpleMessageReceived called, but user does not exist!!!");
	}
}

void GlooxMessageHandler::purpleConversationWriteIM(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			m_stats->messageFromLegacy();
			user->handleWriteIM(conv, who, message, flags, mtime);
		}
		else {
			Log(user->jid(), "purpleConversationWriteIM called for unconnected user..." << message);
		}
	}
	else {
		Log("purple", "purpleConversationWriteIM called, but user does not exist!!!");
	}
}

void GlooxMessageHandler::notifyEmail(PurpleConnection *gc,const char *subject, const char *from,const char *to, const char *url) {
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user!=NULL) {
		if (user->getSetting<bool>("enable_notify_email")) {
			std::string text("New email, ");
			if (from)
				text += "From: " + std::string(from);
			if (to)
				text += ", To: " + std::string(to);
			if (to || from)
				text += "\n";
			if (subject)
				text += "Subject: " + std::string(subject) + "\n";
			if (url)
				text += "URL: " + std::string(url);
			Message s(Message::Chat, user->jid(), text);
			s.setFrom(jid());
			Transport::instance()->send(s.tag());
		}
	}
}

void GlooxMessageHandler::purpleConversationWriteChat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			m_stats->messageFromLegacy();
			user->handleWriteChat(conv, who, message, flags, mtime);
		}
		else {
			Log(user->jid(), "purpleConversationWriteIM called for unconnected user...");
		}
	}
	else {
		Log("purple", "purpleConversationWriteIM called, but user does not exist!!!");
	}
}

void GlooxMessageHandler::purpleConversationDestroyed(PurpleConversation *conv) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		user->purpleConversationDestroyed(conv);
	}
}

void GlooxMessageHandler::purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			user->purpleChatTopicChanged(conv, who, topic);
		}
	}
}

void GlooxMessageHandler::purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			user->purpleChatAddUsers(conv, cbuddies, new_arrivals);
		}
	}
}

void GlooxMessageHandler::purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()){
			user->purpleChatRenameUser(conv, old_name, new_name, new_alias);
		}
	}
}

void GlooxMessageHandler::purpleChatRemoveUsers(PurpleConversation *conv, GList *users) {
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			user->purpleChatRemoveUsers(conv, users);
		}
	}
}

// void GlooxMessageHandler::handleLog(LogLevel level, LogArea area, const std::string &message) {
// // 	if (m_configuration.logAreas & LOG_AREA_XML) {
// // 		if (area == LogAreaXmlIncoming)
// // 			Log("XML IN", message);
// // 		else
// // 			Log("XML OUT", message);
// // 	}
// }

// void GlooxMessageHandler::onSessionCreateError(const Error *error) {
// 	Log("gloox", "sessionCreateError");
//

static bool initPurple(Configuration &cfg) {
	bool ret;

	purple_util_set_user_dir(cfg.userDir.c_str());

	if (cfg.logAreas & LOG_AREA_PURPLE)
		purple_debug_set_ui_ops(&debugUiOps);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps(cfg));

	ret = purple_core_init(PURPLE_UI);
	if (ret) {
		static int conversation_handle;
		static int conn_handle;
		static int blist_handle;

		purple_set_blist(purple_blist_new());
		purple_blist_load();

		purple_prefs_load();

		/* Good default preferences */
		/* The combination of these two settings mean that libpurple will never
		 * (of its own accord) set all the user accounts idle.
		 */
		purple_prefs_set_bool("/purple/away/away_when_idle", false);
		/*
		 * This must be set to something not "none" for idle reporting to work
		 * for, e.g., the OSCAR prpl. We don't implement the UI ops, so this is
		 * okay for now.
		 */
		purple_prefs_set_string("/purple/away/idle_reporting", "system");

		/* Disable all logging */
		purple_prefs_set_bool("/purple/logging/log_ims", false);
		purple_prefs_set_bool("/purple/logging/log_chats", false);
		purple_prefs_set_bool("/purple/logging/log_system", false);


		purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &conversation_handle, PURPLE_CALLBACK(newMessageReceived), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typed", &conversation_handle, PURPLE_CALLBACK(buddyTyped), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(User::signedOnCallback), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &blist_handle,PURPLE_CALLBACK(buddySignedOn), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &blist_handle,PURPLE_CALLBACK(buddySignedOff), NULL);
		purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &blist_handle,PURPLE_CALLBACK(buddyStatusChanged), NULL);
		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", &blist_handle,PURPLE_CALLBACK(NodeRemoved), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);

		purple_commands_init();

	}
	return ret;
}

GlooxMessageHandler* GlooxMessageHandler::m_pInstance = NULL;

static void start_transport(const std::string &config) {
	Configuration c = loadConfigFile(config);
	if (!c)
		return;

	g_thread_init(NULL);

	if (!list_purple_settings) {
		if (!nodaemon)
			daemonize(c.userDir.c_str());

#ifndef WIN32
		if (!nodaemon) {
			int logfd = open("/dev/null", O_WRONLY | O_CREAT, 0644);
			if (logfd <= 0) {
				std::cout << "Can't open log file\n";
				exit(1);
			}
			if (dup2(logfd, STDERR_FILENO) < 0) {
				std::cout << "Can't redirect stderr\n";
				exit(1);
			}
			if (dup2(logfd, STDOUT_FILENO) < 0) {
				std::cout << "Can't redirect stdout\n";
				exit(1);
			}
			close(logfd);
		}
#endif
	}

	if (list_purple_settings) {
		c.logAreas = 0;
	}
	
	if (!initPurple(c))
		return;

	AbstractProtocol *protocol = loadProtocol(c);
	if (!protocol)
		return;


	Transport *transport = new Transport(c, protocol);


	AbstractBackend *sql = NULL;
	if (check_db_version) {
		sql = new SQLClass(upgrade_db, true);
		if (!sql->loaded())
			exit(3);
	}
	if (upgrade_db) {
		sql = new SQLClass(upgrade_db);
		if (!sql->loaded()) {
			delete sql;
			return;
		}
	}

	GMainLoop *loop = NULL;
	if (CONFIG().eventloop == "glib") {
		loop = g_main_loop_new(NULL, FALSE);
	}
#ifdef WITH_LIBEVENT
	else {
		struct event_base *base = (struct event_base *) event_init();
	}
#endif


	if (list_purple_settings) {
		listPurpleSettings();
		return;
	}

	sql = new SQLClass(upgrade_db);
	if (!sql->loaded()) {
		return;
	}

	transport->setSQLBackend(sql);
	transport->transportConnect();
	
	if (loop) {
		g_main_loop_run(loop);
	}
#ifdef WITH_LIBEVENT
	else {
		event_loop(0);
	}
#endif
}

static void spectrum_sigint_handler(int sig) {
	delete Transport::instance();
}

static void spectrum_sigterm_handler(int sig) {
	delete Transport::instance();
}

#ifndef WIN32
static void spectrum_sigchld_handler(int sig)
{
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WNOHANG);
	} while (pid != 0 && pid != (pid_t)-1);

	if ((pid == (pid_t) - 1) && (errno != ECHILD)) {
		char errmsg[BUFSIZ];
		snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
		perror(errmsg);
	}
}
static void spectrum_sighup_handler(int sig) {
	Configuration c = loadConfigFile(CONFIG().file);
	if (c)
		Transport::instance()->setConfiguration(c);
}
#endif

int main( int argc, char* argv[] ) {
	GError *error = NULL;
	GOptionContext *context;
	context = g_option_context_new("config_file_name or profile name");
	g_option_context_add_main_entries(context, options_entries, "");
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		std::cout << "option parsing failed: " << error->message << "\n";
		return -1;
	}

	if (ver) {
		std::cout << VERSION << "\n";
		g_option_context_free(context);
		return 0;
	}

	if (argc != 2) {
#ifdef WIN32
		std::cout << "Usage: spectrum.exe <configuration_file.cfg>\n";
#else

#if GLIB_CHECK_VERSION(2,14,0)
	std::cout << g_option_context_get_help(context, FALSE, NULL);
#else
	std::cout << "Usage: spectrum <configuration_file.cfg>\n";
	std::cout << "See \"man spectrum\" for more info.\n";
#endif
		
#endif
	}
	else {
#ifndef WIN32
		signal(SIGPIPE, SIG_IGN);

		if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
			std::cout << "SIGCHLD handler can't be set\n";
			g_option_context_free(context);
			return -1;
		}

		if (signal(SIGINT, spectrum_sigint_handler) == SIG_ERR) {
			std::cout << "SIGINT handler can't be set\n";
			g_option_context_free(context);
			return -1;
		}

		if (signal(SIGTERM, spectrum_sigterm_handler) == SIG_ERR) {
			std::cout << "SIGTERM handler can't be set\n";
			g_option_context_free(context);
			return -1;
		}

		struct sigaction sa;
		memset(&sa, 0, sizeof(sa)); 
		sa.sa_handler = spectrum_sighup_handler;
		if (sigaction(SIGHUP, &sa, NULL)) {
			std::cout << "SIGHUP handler can't be set\n";
			g_option_context_free(context);
			return -1;
		}
#endif
		std::string config(argv[1]);
		start_transport(config);
	}
	g_option_context_free(context);
	return 0;
}

