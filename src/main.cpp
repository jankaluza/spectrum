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
// #include "discoinfohandler.h"
#include "statshandler.h"
#include "vcardhandler.h"
#include "gatewayhandler.h"
#include "capabilityhandler.h"
#include "configfile.h"
#include "spectrum_util.h"
#include "sql.h"
#include "user.h"
#include "filetransfermanager.h"
#include "localization.h"
#include "spectrumbuddy.h"
#include "spectrumnodehandler.h"
#include "transport.h"

#include "parser.h"
#include "commands.h"
#include "protocols/abstractprotocol.h"
#include "protocols/aim.h"
#include "protocols/facebook.h"
#include "protocols/gg.h"
#include "protocols/icq.h"
#include "protocols/irc.h"
#include "protocols/msn.h"
#include "protocols/msn_pecan.h"
#include "protocols/myspace.h"
#include "protocols/qq.h"
#include "protocols/simple.h"
#include "protocols/twitter.h"
#include "protocols/xmpp.h"
#include "protocols/yahoo.h"
#include "protocols/sipe.h"
#include "cmds.h"

#include "gloox/adhoc.h"
#include "gloox/error.h"
#include <gloox/tlsbase.h>
#include <gloox/compressionbase.h>
#include <gloox/mucroom.h>
#include <gloox/sha.h>
#include <gloox/vcardupdate.h>
#include <gloox/base64.h>

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
		std::cout << "EE cannot write to lock file " << lock_file << ". Exiting\n";
		exit(1);
    }
	sprintf(process_pid,"%d\n",getpid());
	fwrite(process_pid,1,strlen(process_pid),lock_file_f);
	fclose(lock_file_f);
	
	freopen( "/dev/null", "r", stdin);
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

/*
 * Called when user is renamed
 */
static void conv_chat_rename_user(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	GlooxMessageHandler::instance()->purpleChatRenameUser(conv, old_name, new_name, new_alias);
}

/*
 * Called when users are removed from chat
 */
static void conv_chat_remove_users(PurpleConversation *conv, GList *users) {
	GlooxMessageHandler::instance()->purpleChatRemoveUsers(conv, users);
}

/*
 * Called when user is logged in...
 */
static void signed_on(PurpleConnection *gc,gpointer unused) {
	GlooxMessageHandler::instance()->signedOn(gc, unused);
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
		return NULL;
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
		bool handled = GlooxMessageHandler::instance()->protocol()->onPurpleRequestInput(user, title, primary, secondary, default_value, multiline, masked, hint, ok_text, ok_cb, cancel_text, cancel_cb, account, who, conv, user_data);
		if (handled)
			return NULL;
	}
	Log(user->jid(), "WARNING: purple_request_input not handled. primary == NULL, title ==" << t << ", secondary ==" << s);
	return NULL;
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
	return NULL;
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

	return NULL;
}

static void * requestAction(const char *title, const char *primary,const char *secondary, int default_action,PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data,size_t action_count, va_list actions){
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	bool handled = false;
	if (!user) {
		Log("requestAction", "WARNING: purple_request_action not handled. No user for account =" << purple_account_get_username(account));
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
		GlooxMessageHandler::instance()->fetchVCard(name);
	}
	g_free(data);
	return FALSE;
}

/*
 * Connect user to legacy network
 */
static gboolean connectUser(gpointer data) {
	std::string name((char*)data);
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
	if (user && user->readyForConnect() && !user->isConnected()) {
		user->connect();
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

/*
 * Checking new connections for our gloox proxy...
 */
static gboolean ftServerReceive(gpointer data) {
	if (GlooxMessageHandler::instance()->ftServer->recv(1) == ConnNoError)
		return TRUE;
	return FALSE;
}

static gboolean transportReconnect(gpointer data) {
	GlooxMessageHandler::instance()->transportConnect();
	return FALSE;
}

static gboolean sendPing(gpointer data) {
	GlooxMessageHandler::instance()->j->xmppPing(GlooxMessageHandler::instance()->jid(), GlooxMessageHandler::instance());
	return TRUE;
}

/*
 * Called by notifier when new data can be received from socket
 */
static gboolean transportDataReceived(GIOChannel *source, GIOCondition condition, gpointer data) {
	GlooxMessageHandler::instance()->j->recv(1000);
	return TRUE;
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

GlooxMessageHandler::GlooxMessageHandler(const std::string &config) : MessageHandler(),ConnectionListener(),PresenceHandler(),SubscriptionHandler() {
	m_pInstance = this;
	m_reconnectCount = 0;
	m_config = config;
	m_firstConnection = true;
	ftManager = NULL;
	ft = NULL;
	m_parser = NULL;
	m_sql = NULL;
	m_collector = NULL;
	m_searchHandler = NULL;
	m_vcardManager = NULL;
	m_reg = NULL;
	m_adhoc = NULL;
	gatewayHandler = NULL;
	ftServer = NULL;
	m_stats = NULL;
	connectIO = NULL;
#ifndef WIN32
	m_configInterface = NULL;
#endif

	bool loaded = true;

	if (!loadConfigFile(config))
		loaded = false;

	g_thread_init(NULL);
	if (check_db_version) {
		m_sql = new SQLClass(this, upgrade_db, true);
		if (!m_sql->loaded())
			exit(3);
	}
	if (upgrade_db) {
		m_sql = new SQLClass(this, upgrade_db);
		if (!m_sql->loaded())
			loaded = false;
	}
	
	if (!list_purple_settings) {
		if (loaded && !nodaemon)
			daemonize(configuration().userDir.c_str());

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

	m_transport = new Transport(m_configuration.jid);

	j = new HiComponent("jabber:component:accept",m_configuration.server,m_configuration.jid,m_configuration.password,m_configuration.port);

	if (m_configuration.logAreas & LOG_AREA_PURPLE)
		j->logInstance().registerLogHandler(LogLevelDebug, LogAreaXmlIncoming | LogAreaXmlOutgoing, &Log_);
	
	m_loop = g_main_loop_new(NULL, FALSE);

	m_userManager = new UserManager();
	m_adhoc = new GlooxAdhocHandler();
	m_searchHandler = NULL;

	if (list_purple_settings)
		m_configuration.logAreas = 0;
	
	if (loaded && !initPurple())
		loaded = false;
	
	if (loaded && !loadProtocol())
		loaded = false;

	if (list_purple_settings) {
		listPurpleSettings();
		loaded = false;
	}

	if (loaded) {
		m_sql = new SQLClass(this, upgrade_db);
		if (!m_sql->loaded())
			loaded = false;
	}

	if (loaded) {
#ifndef WIN32
		if (!m_configuration.config_interface.empty())
			m_configInterface = new ConfigInterface(m_configuration.config_interface, j->logInstance());
#endif
		m_discoHandler = new CapabilityHandler();
		m_spectrumNodeHandler = new SpectrumNodeHandler();

// 		m_discoInfoHandler = new GlooxDiscoInfoHandler();
// 		j->registerIqHandler(m_discoInfoHandler,ExtDiscoInfo);
		
		j->registerIqHandler(m_adhoc, ExtAdhocCommand);
		j->registerStanzaExtension( new Adhoc::Command() );
		j->registerStanzaExtension( new MUCRoom::MUC() );
		j->disco()->addFeature( XMLNS_ADHOC_COMMANDS );
		j->disco()->registerNodeHandler( m_adhoc, XMLNS_ADHOC_COMMANDS );
		j->disco()->registerNodeHandler( m_adhoc, std::string() );
		
		m_parser = new GlooxParser();
		m_collector = new AccountCollector();

		ftManager = new FileTransferManager();
		ft = new SIProfileFT(j, ftManager);
		ftManager->setSIProfileFT(ft);
		ftServer = new SOCKS5BytestreamServer(j->logInstance(), m_configuration.filetransfer_proxy_port, m_configuration.filetransfer_proxy_ip);
		if( ftServer->listen() != ConnNoError ) {}
		ft->addStreamHost( j->jid(), m_configuration.filetransfer_proxy_streamhost_ip, m_configuration.filetransfer_proxy_streamhost_port);
		ft->registerSOCKS5BytestreamServer( ftServer );
		if (m_configuration.transportFeatures & TRANSPORT_FEATURE_FILETRANSFER) {
			purple_timeout_add(50, &ftServerReceive, NULL);
		}

		j->registerMessageHandler(this);
		j->registerConnectionListener(this);
		gatewayHandler = new GlooxGatewayHandler(this);
		j->registerIqHandler(gatewayHandler, ExtGateway);
		m_reg = new GlooxRegisterHandler();
		j->registerIqHandler(m_reg, ExtRegistration);
		m_stats = new GlooxStatsHandler(this);
		j->registerIqHandler(m_stats, ExtStats);
		m_vcardManager = new VCardManager(j);
#ifndef WIN32
                if (m_configInterface)
		m_configInterface->registerHandler(m_stats);
#endif
		m_vcard = new GlooxVCardHandler(this);
		j->registerIqHandler(m_vcard, ExtVCard);
		j->registerPresenceHandler(this);
		j->registerSubscriptionHandler(this);
		Transport::instance()->registerStanzaExtension( new VCardUpdate );

		transportConnect();

		g_main_loop_run(m_loop);
	}
}

GlooxMessageHandler::~GlooxMessageHandler(){
	delete m_userManager;
	purple_blist_uninit();
	purple_core_quit();
	g_main_loop_quit(m_loop);
	g_main_loop_unref(m_loop);
	
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
	delete j;
}

bool GlooxMessageHandler::loadProtocol(){
	if (configuration().protocol == "aim")
		m_protocol = (AbstractProtocol*) new AIMProtocol(this);
	else if (configuration().protocol == "facebook")
		m_protocol = (AbstractProtocol*) new FacebookProtocol(this);
	else if (configuration().protocol == "twitter")
		m_protocol = (AbstractProtocol*) new TwitterProtocol(this);
	else if (configuration().protocol == "gg")
		m_protocol = (AbstractProtocol*) new GGProtocol(this);
	else if (configuration().protocol == "icq")
		m_protocol = (AbstractProtocol*) new ICQProtocol(this);
	else if (configuration().protocol == "irc")
		m_protocol = (AbstractProtocol*) new IRCProtocol(this);
	else if (configuration().protocol == "msn")
		m_protocol = (AbstractProtocol*) new MSNProtocol(this);
	else if (configuration().protocol == "msn_pecan")
		m_protocol = (AbstractProtocol*) new MSNPecanProtocol(this);
	else if (configuration().protocol == "myspace")
		m_protocol = (AbstractProtocol*) new MyspaceProtocol(this);
	else if (configuration().protocol == "qq")
		m_protocol = (AbstractProtocol*) new QQProtocol(this);
	else if (configuration().protocol == "simple")
		m_protocol = (AbstractProtocol*) new SimpleProtocol(this);
	else if (configuration().protocol == "xmpp")
		m_protocol = (AbstractProtocol*) new XMPPProtocol(this);
	else if (configuration().protocol == "yahoo")
		m_protocol = (AbstractProtocol*) new YahooProtocol(this);
	else if (configuration().protocol == "sipe")
		m_protocol = (AbstractProtocol*) new SIPEProtocol(this);
	else {
		Log("loadProtocol", "Protocol \"" << configuration().protocol << "\" does not exist.");
		Log("loadProtocol", "Protocol has to be one of: facebook, gg, msn, irc, xmpp, myspace, qq, simple, aim, yahoo.");
		return false;
	}

	if (!purple_find_prpl(m_protocol->protocol().c_str())) {
		Log("loadProtocol", "There is no libpurple plugin installed for protocol \"" << configuration().protocol << "\"");
		return false;
	}

	if (!m_protocol->userSearchAction().empty()) {
		m_searchHandler = new GlooxSearchHandler(this);
		j->registerIqHandler(m_searchHandler, ExtSearch);
	}
	
	if (m_configuration.encoding.empty())
		m_configuration.encoding = m_protocol->defaultEncoding();

	ConfigFile cfg(m_config);
	cfg.loadPurpleAccountSettings(m_configuration);
	
	return true;
}

void GlooxMessageHandler::handleLog(LogLevel level, LogArea area, const std::string &message) {
// 	if (m_configuration.logAreas & LOG_AREA_XML) {
// 		if (area == LogAreaXmlIncoming)
// 			Log("XML IN", message);
// 		else
// 			Log("XML OUT", message);
// 	}
}

void GlooxMessageHandler::onSessionCreateError(const Error *error) {
	Log("gloox", "sessionCreateError");
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
			if (text){
				Message s(Message::Chat, user->jid(), tr(user->getLang(), text));
				std::string from;
				s.setFrom(jid());
				j->send(s);
			}
			m_userManager->removeUserTimer(user);
		}
		else {
			if (user->reconnectCount() > 0) {
				if (text) {
					Message s(Message::Chat, user->jid(), tr(user->getLang(), text));
					std::string from;
					s.setFrom(jid());
					j->send(s);
				}
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

void GlooxMessageHandler::signedOn(PurpleConnection *gc, gpointer unused) {
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = (User *) userManager()->getUserByAccount(account);
	if (user != NULL) {
		Log(user->jid(), "Logged in to legacy network");
		purple_timeout_add_seconds(30, &getVCard, g_strdup(user->jid().c_str()));
		user->connected();
	}
}

void GlooxMessageHandler::purpleAuthorizeClose(void *data) {
	authRequest *d = (authRequest *) data;
	if (!d)
		return;
	User *user = (User *) userManager()->getUserByAccount(d->account);
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

bool GlooxMessageHandler::loadConfigFile(const std::string &config) {
	ConfigFile cfg(config.empty() ? m_config : config);
	Configuration c = cfg.getConfiguration();

	if (m_configuration)
		cfg.loadPurpleAccountSettings(c);
	
	if (!c && !m_configuration)
		return false;
	if (c)
		m_configuration = c;

	if (logfile)
		m_configuration.logfile = std::string(logfile);
	
	if (!m_configuration.logfile.empty())
		Log_.setLogFile(m_configuration.logfile);

	if (!lock_file)
		lock_file = g_strdup(m_configuration.pid_f.c_str());
	
	return true;
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
	if (protocol()->notifyUsername().empty())
		return;
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = (User *) userManager()->getUserByAccount(account);
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
			s.setFrom(protocol()->notifyUsername() + "@" + jid() + "/bot");
			j->send(s);
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

void GlooxMessageHandler::handleSubscription(const Subscription &stanza) {
	// answer to subscibe
	if(stanza.subtype() == Subscription::Subscribe && stanza.to().username() == "") {
		Log(stanza.from().full(), "Subscribe presence received => sending subscribed");
		Tag *reply = new Tag("presence");
		reply->addAttribute( "to", stanza.from().bare() );
		reply->addAttribute( "from", stanza.to().bare() );
		reply->addAttribute( "type", "subscribed" );
		j->send( reply );
		return;
	}

	User *user;
	if (protocol()->tempAccountsAllowed()) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		user = (User *) userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = (User *) userManager()->getUserByJID(stanza.from().bare());
	}
	if (user)
		user->handleSubscription(stanza);
	else
		Log(stanza.from().full(), "Subscribe presence received, but this user is not logged in");

}

void GlooxMessageHandler::handlePresence(const Presence &stanza){
	if (stanza.subtype() == Presence::Error) {
		return;
	}

	if (!isValidNode(stanza.from().username())) {
		Tag *tag = new Tag("presence");
		tag->addAttribute("to", stanza.from().full());
		tag->addAttribute("from", stanza.to().full());
		tag->addAttribute("type", "error");

		Tag *error = new Tag("error");
		error->addAttribute("type", "modify");

		Tag *jid = new Tag("jid-malformed");
		jid->addAttribute("xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas");
		
		error->addChild(jid);
		tag->addChild(error);
		j->send(tag);
		return;
	}
	
	// get entity capabilities
	Tag *c = NULL;
	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
	Log(stanza.from().full(), "Presence received (" << (int)stanza.subtype() << ") for: " << stanza.to().full() << "isMUC" << isMUC);
	

	if (stanza.presence() != Presence::Unavailable && ((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || (isMUC && protocol()->tempAccountsAllowed()))) {
		Tag *stanzaTag = stanza.tag();
		if (!stanzaTag) return;
		Tag *c = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/caps");
		Log(stanza.from().full(), "asking for caps/disco#info");
		// Presence has caps and caps are not cached.
		if (c != NULL && !Transport::instance()->hasClientCapabilities(c->findAttribute("ver"))) {
			int context = m_discoHandler->waitForCapabilities(c->findAttribute("ver"), stanza.to().full());
			std::string node = c->findAttribute("node") + std::string("#") + c->findAttribute("ver");;
			j->disco()->getDiscoInfo(stanza.from(), node, m_discoHandler, context, j->getID());
		}
		else {
			int context = m_discoHandler->waitForCapabilities(stanza.from().full(), stanza.to().full());
			j->disco()->getDiscoInfo(stanza.from(), "", m_discoHandler, context, j->getID());
		}
		delete stanzaTag;
	}
	User *user;
	std::string userkey;
	if (protocol()->tempAccountsAllowed()) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		userkey = stanza.from().bare() + server;
		user = (User *) userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = (User *) userManager()->getUserByJID(stanza.from().bare());
		userkey = stanza.from().bare();
	}
	if (user == NULL) {
		// we are not connected and probe arrived => answer with unavailable
		if (stanza.subtype() == Presence::Probe) {
			Log(stanza.from().full(), "Answering to probe presence with unavailable presence");
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", stanza.from().full());
			tag->addAttribute("from", stanza.to().bare() + "/bot");
			tag->addAttribute("type", "unavailable");
			j->send(tag);
		}
		else if (((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || ( protocol()->tempAccountsAllowed() && isMUC)) && stanza.presence() != Presence::Unavailable){
			UserRow res = sql()->getUserByJid(userkey);
			if(res.id==-1 && !protocol()->tempAccountsAllowed()) {
				// presence from unregistered user
				Log(stanza.from().full(), "This user is not registered");
				return;
			}
			else {
				if(res.id==-1 && protocol()->tempAccountsAllowed()) {
					sql()->addUser(userkey,stanza.from().username(),"","en",m_configuration.encoding);
					res = sql()->getUserByJid(userkey);
				}
				bool isVip = res.vip;
				std::list<std::string> const &x = configuration().allowedServers;
				if (configuration().onlyForVIP && !isVip && std::find(x.begin(), x.end(), stanza.from().server()) == x.end()) {
					Log(stanza.from().full(), "This user is not VIP, can't login...");
					return;
				}
				Log(stanza.from().full(), "Creating new User instance");
				if (protocol()->tempAccountsAllowed()) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					user = new User(this, stanza.from(), stanza.to().resource() + "@" + server, "", stanza.from().bare() + server, res.id, res.encoding, res.language, res.vip);
				}
				else {
					if (purple_accounts_find(res.uin.c_str(), protocol()->protocol().c_str()) != NULL) {
						PurpleAccount *act = purple_accounts_find(res.uin.c_str(), protocol()->protocol().c_str());
						user = (User *) userManager()->getUserByAccount(act);
						if (user) {
							Log(stanza.from().full(), "This account is already connected by another jid " << user->jid());
							return;
						}
					}
					user = new User(this, stanza.from(), res.uin, res.password, stanza.from().bare(), res.id, res.encoding, res.language, res.vip);
				}
				user->setFeatures(isVip ? configuration().VIPFeatures : configuration().transportFeatures);
				if (c != NULL)
					if (Transport::instance()->hasClientCapabilities(c->findAttribute("ver")))
						user->setResource(stanza.from().resource(), stanza.priority(), Transport::instance()->getCapabilities(c->findAttribute("ver")));

				m_userManager->addUser(user);
				user->receivedPresence(stanza);
				if (protocol()->tempAccountsAllowed()) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					server = stanza.from().bare() + server;
					purple_timeout_add_seconds(15, &connectUser, g_strdup(server.c_str()));
				}
				else
					purple_timeout_add_seconds(15, &connectUser, g_strdup(stanza.from().bare().c_str()));
			}
		}
		if (stanza.presence() == Presence::Unavailable && stanza.to().username() == ""){
			Log(stanza.from().full(), "User is already logged out => sending unavailable presence");
			Tag *tag = new Tag("presence");
			tag->addAttribute( "to", stanza.from().bare() );
			tag->addAttribute( "type", "unavailable" );
			tag->addAttribute( "from", jid() );
			j->send( tag );
		}
	}
	else {
		user->receivedPresence(stanza);
	}
	if (stanza.to().username() == "" && user != NULL) {
		if(stanza.presence() == Presence::Unavailable && user->isConnected() == true && user->getResources().empty()) {
			Log(stanza.from().full(), "Logging out");
			m_userManager->removeUser(user);
		}
		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false && user->getResources().empty()) {
			Log(stanza.from().full(), "Logging out, but he's not connected...");
			m_userManager->removeUser(user);
		}
// 		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false) {
// 			Log(stanza.from().full(), "Can't logout because we're connecting now...");
// 		}
	}
	else if (user == NULL && stanza.to().username() == "" && stanza.presence() == Presence::Unavailable) {
		UserRow res = sql()->getUserByJid(userkey);
		if (res.id != -1) {
			sql()->setUserOnline(res.id, false);
		}
	}
}

void GlooxMessageHandler::onConnect() {
	Log("gloox", "CONNECTED!");
	m_reconnectCount = 0;

	if (m_firstConnection) {
		j->disco()->setIdentity("gateway", protocol()->gatewayIdentity(), configuration().discoName);
		j->disco()->setVersion(configuration().discoName, VERSION, "");

		std::string id = "gateway";
		id += '/';
		id += protocol()->gatewayIdentity();
		id += '/';
		id += '/';
		id += "Spectrum";
		id += '<';
		
		std::list<std::string> features = protocol()->transportFeatures();
		features.sort();
		for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
			j->disco()->addFeature(*it);
		}

		std::list<std::string> f = protocol()->buddyFeatures();
		if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#items") == f.end())
			f.push_back("http://jabber.org/protocol/disco#items");
		if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#info") == f.end())
			f.push_back("http://jabber.org/protocol/disco#info");
		f.sort();
		for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
			id += (*it);
			id += '<';
		}

		SHA sha;
		sha.feed( id );
		m_configuration.hash = Base64::encode64( sha.binary() );
		j->disco()->registerNodeHandler( m_spectrumNodeHandler, "http://spectrum.im/transport#" + m_configuration.hash );

		if (m_configuration.protocol != "irc")
			new AutoConnectLoop();
		m_firstConnection = false;
		purple_timeout_add_seconds(60, &sendPing, this);
	}
}

void GlooxMessageHandler::onDisconnect(ConnectionError e) {
	Log("gloox", "Disconnected from Jabber server !");
	switch (e) {
		case ConnNoError: Log("gloox", "Reason: No error"); break;
		case ConnStreamError: Log("gloox", "Reason: Stream error"); break;
		case ConnStreamVersionError: Log("gloox", "Reason: Stream version error"); break;
		case ConnStreamClosed: Log("gloox", "Reason: Stream closed"); break;
		case ConnProxyAuthRequired: Log("gloox", "Reason: Proxy auth required"); break;
		case ConnProxyAuthFailed: Log("gloox", "Reason: Proxy auth failed"); break;
		case ConnProxyNoSupportedAuth: Log("gloox", "Reason: Proxy no supported auth"); break;
		case ConnIoError: Log("gloox", "Reason: IO Error"); break;
		case ConnParseError: Log("gloox", "Reason: Parse error"); break;
		case ConnConnectionRefused: Log("gloox", "Reason: Connection refused"); break;
// 		case ConnSocketError: Log("gloox", "Reason: Socket error"); break;
		case ConnDnsError: Log("gloox", "Reason: DNS Error"); break;
		case ConnOutOfMemory: Log("gloox", "Reason: Out Of Memory"); break;
		case ConnNoSupportedAuth: Log("gloox", "Reason: No supported auth"); break;
		case ConnTlsFailed: Log("gloox", "Reason: Tls failed"); break;
		case ConnTlsNotAvailable: Log("gloox", "Reason: Tls not available"); break;
		case ConnCompressionFailed: Log("gloox", "Reason: Compression failed"); break;
// 		case ConnCompressionNotAvailable: Log("gloox", "Reason: Compression not available"); break;
		case ConnAuthenticationFailed: Log("gloox", "Reason: Authentication Failed"); break;
		case ConnUserDisconnected: Log("gloox", "Reason: User disconnected"); break;
		case ConnNotConnected: Log("gloox", "Reason: Not connected"); break;
	};

	switch (j->streamError()) {
		case StreamErrorBadFormat: Log("gloox", "Stream error: Bad format"); break;
		case StreamErrorBadNamespacePrefix: Log("gloox", "Stream error: Bad namespace prefix"); break;
		case StreamErrorConflict: Log("gloox", "Stream error: Conflict"); break;
		case StreamErrorConnectionTimeout: Log("gloox", "Stream error: Connection timeout"); break;
		case StreamErrorHostGone: Log("gloox", "Stream error: Host gone"); break;
		case StreamErrorHostUnknown: Log("gloox", "Stream error: Host unknown"); break;
		case StreamErrorImproperAddressing: Log("gloox", "Stream error: Improper addressing"); break;
		case StreamErrorInternalServerError: Log("gloox", "Stream error: Internal server error"); break;
		case StreamErrorInvalidFrom: Log("gloox", "Stream error: Invalid from"); break;
		case StreamErrorInvalidId: Log("gloox", "Stream error: Invalid ID"); break;
		case StreamErrorInvalidNamespace: Log("gloox", "Stream error: Invalid Namespace"); break;
		case StreamErrorInvalidXml: Log("gloox", "Stream error: Invalid XML"); break;
		case StreamErrorNotAuthorized: Log("gloox", "Stream error: Not Authorized"); break;
		case StreamErrorPolicyViolation: Log("gloox", "Stream error: Policy violation"); break;
		case StreamErrorRemoteConnectionFailed: Log("gloox", "Stream error: Remote connection failed"); break;
		case StreamErrorResourceConstraint: Log("gloox", "Stream error: Resource constraint"); break;
		case StreamErrorRestrictedXml: Log("gloox", "Stream error: Restricted XML"); break;
		case StreamErrorSeeOtherHost: Log("gloox", "Stream error: See other host"); break;
		case StreamErrorSystemShutdown: Log("gloox", "Stream error: System shutdown"); break;
		case StreamErrorUndefinedCondition: Log("gloox", "Stream error: Undefined Condition"); break;
		case StreamErrorUnsupportedEncoding: Log("gloox", "Stream error: Unsupported encoding"); break;
		case StreamErrorUnsupportedStanzaType: Log("gloox", "Stream error: Unsupported stanza type"); break;
		case StreamErrorUnsupportedVersion: Log("gloox", "Stream error: Unsupported version"); break;
		case StreamErrorXmlNotWellFormed: Log("gloox", "Stream error: XML Not well formed"); break;
		case StreamErrorUndefined: Log("gloox", "Stream error: Error undefined"); break;
	};

	switch (j->authError()) {
		case AuthErrorUndefined: Log("gloox", "Auth error: Error undefined"); break;
		case SaslAborted: Log("gloox", "Auth error: Sasl aborted"); break;
		case SaslIncorrectEncoding: Log("gloox", "Auth error: Sasl incorrect encoding"); break;        
		case SaslInvalidAuthzid: Log("gloox", "Auth error: Sasl invalid authzid"); break;
		case SaslInvalidMechanism: Log("gloox", "Auth error: Sasl invalid mechanism"); break;
		case SaslMalformedRequest: Log("gloox", "Auth error: Sasl malformed request"); break;
		case SaslMechanismTooWeak: Log("gloox", "Auth error: Sasl mechanism too weak"); break;
		case SaslNotAuthorized: Log("gloox", "Auth error: Sasl Not authorized"); break;
		case SaslTemporaryAuthFailure: Log("gloox", "Auth error: Sasl temporary auth failure"); break;
		case NonSaslConflict: Log("gloox", "Auth error: Non sasl conflict"); break;
		case NonSaslNotAcceptable: Log("gloox", "Auth error: Non sasl not acceptable"); break;
		case NonSaslNotAuthorized: Log("gloox", "Auth error: Non sasl not authorized"); break;
	};

	if (m_reconnectCount == 1)
		m_userManager->removeAllUsers();

	Log("gloox", "trying to reconnect after 3 seconds");
	purple_timeout_add_seconds(3, &transportReconnect, NULL);

	if (connectIO) {
		g_source_remove(connectID);
		connectIO = NULL;
	}
}

void GlooxMessageHandler::transportConnect() {
	m_reconnectCount++;
	if (m_sql->loaded()) {
		j->connect(false);
		int mysock = dynamic_cast<ConnectionTCPClient*>( j->connectionImpl() )->socket();
		if (mysock > 0) {
			connectIO = g_io_channel_unix_new(mysock);
			connectID = g_io_add_watch(connectIO, (GIOCondition) READ_COND, &transportDataReceived, NULL);
		}
	}
	else {
		Log("gloox", "Tried to reconnect, but database is not ready yet.");
		Log("gloox", "trying to reconnect after 3 seconds");
		purple_timeout_add_seconds(3, &transportReconnect, NULL);
	}
}

bool GlooxMessageHandler::onTLSConnect(const CertInfo & info) {
	return false;
}

void GlooxMessageHandler::handleMessage (const Message &msg, MessageSession *session) {
	if (msg.from().bare() == msg.to().bare())
		return;
	if (msg.subtype() == Message::Error || msg.subtype() == Message::Invalid)
		return;
	User *user;
	// TODO; move it to IRCProtocol
	if (m_configuration.protocol == "irc") {
		std::string server = msg.to().username().substr(msg.to().username().find("%") + 1, msg.to().username().length() - msg.to().username().find("%"));
		user = (User *) userManager()->getUserByJID(msg.from().bare() + server);
	}
	else {
		user = (User *) userManager()->getUserByJID(msg.from().bare());
	}
	if (user!=NULL) {
		if (user->isConnected()) {
			Tag *msgTag = msg.tag();
			if (!msgTag) return;
			Tag *chatstates = msgTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/chatstates");
			if (chatstates != NULL) {
// 				std::string username = msg.to().username();
// 				std::for_each( username.begin(), username.end(), replaceJidCharacters() );
				user->handleChatState(purpleUsername(msg.to().username()), chatstates->name());
			}
			if (msgTag->findChild("body") != NULL) {
				m_stats->messageFromJabber();
				user->handleMessage(msg);
			}
			delete msgTag;
		}
		else{
			Log(msg.from().bare(), "New message received, but we're not connected yet");
		}
	}
	else {
		Tag *msgTag = msg.tag();
		if (!msgTag) return;
		if (msgTag->findChild("body") != NULL) {
			Message s(Message::Error, msg.from().full(), msg.body());
			s.setFrom(msg.to().full());
			Error *c = new Error(StanzaErrorTypeWait, StanzaErrorRecipientUnavailable);
			c->setText(tr(configuration().language.c_str(),_("This message couldn't be sent, because you are not connected to legacy network. You will be automatically reconnected soon.")));
			s.addExtension(c);
			j->send(s);

			Tag *stanza = new Tag("presence");
			stanza->addAttribute( "to", msg.from().bare());
			stanza->addAttribute( "type", "probe");
			stanza->addAttribute( "from", jid());
			j->send(stanza);
		}
		delete msgTag;
	}

}

void GlooxMessageHandler::handleVCard(const JID& jid, const VCard* vcard) {
	if (!vcard)
		return;
	User *user = (User *) GlooxMessageHandler::instance()->userManager()->getUserByJID(jid.bare());
	if (user && user->isConnected()) {
		user->handleVCard(vcard);
	}
}

void GlooxMessageHandler::handleVCardResult(VCardContext context, const JID& jid, StanzaError se) {
	
}

bool GlooxMessageHandler::initPurple(){
	bool ret;

	purple_util_set_user_dir(configuration().userDir.c_str());

	if (m_configuration.logAreas & LOG_AREA_PURPLE)
		purple_debug_set_ui_ops(&debugUiOps);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

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
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
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

static void spectrum_sigint_handler(int sig) {
	delete GlooxMessageHandler::instance();
}

static void spectrum_sigterm_handler(int sig) {
	delete GlooxMessageHandler::instance();
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
	GlooxMessageHandler::instance()->loadConfigFile();
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
		new GlooxMessageHandler(config);
	}
	g_option_context_free(context);
}

