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
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "utf8.h"
#include "log.h"
#include "geventloop.h"
#include "accountcollector.h"
#include "autoconnectloop.h"
#include "usermanager.h"
#include "adhoc/adhochandler.h"
#include "adhoc/adhocrepeater.h"
#include "searchhandler.h"
#include "searchrepeater.h"
#include "filetransferrepeater.h"
#include "registerhandler.h"
#include "discoinfohandler.h"
#include "statshandler.h"
#include "vcardhandler.h"
#include "gatewayhandler.h"
#include "caps.h"
#include "spectrum_util.h"
#include "sql.h"
#include "user.h"
#include "filetransfermanager.h"
#include "localization.h"

#include "parser.h"
#include "commands.h"
#include "protocols/abstractprotocol.h"
#include "protocols/aim.h"
#include "protocols/facebook.h"
#include "protocols/gg.h"
#include "protocols/icq.h"
#include "protocols/irc.h"
#include "protocols/msn.h"
#include "protocols/myspace.h"
#include "protocols/qq.h"
#include "protocols/simple.h"
#include "protocols/xmpp.h"
#include "protocols/yahoo.h"
#include "cmds.h"

#include <gloox/tlsbase.h>
#include <gloox/compressionbase.h>
#include <gloox/sha.h>
#include <gloox/base64.h>

static gboolean nodaemon = FALSE;
static gchar *logfile = NULL;
static gchar *lock_file = NULL;
static gboolean ver = FALSE;
static gboolean upgrade_db = FALSE;

static GOptionEntry options_entries[] = {
	{ "nodaemon", 'n', 0, G_OPTION_ARG_NONE, &nodaemon, "Disable background daemon mode", NULL },
	{ "logfile", 'l', 0, G_OPTION_ARG_STRING, &logfile, "Set file to log", NULL },
	{ "pidfile", 'p', 0, G_OPTION_ARG_STRING, &lock_file, "File where to write transport PID", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Shows Spectrum version", NULL },
	{ "upgrade-db", 'u', 0, G_OPTION_ARG_NONE, &upgrade_db, "Upgrades Spectrum database", NULL },
	{ NULL }
};

static void daemonize(void) {
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
	if ((chdir("/")) < 0) {
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
};

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

static void NodeRemoved(PurpleBlistNode *node, gpointer null) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	if (buddy->node.ui_data) {
		long *id = (long *) buddy->node.ui_data;
		delete id;
	}
}

/*
 * Called when purple disconnects from legacy network.
 */
void connection_report_disconnect(PurpleConnection *gc,PurpleConnectionError reason,const char *text){
	GlooxMessageHandler::instance()->purpleConnectionError(gc, reason, text);
}

/*
 * Called when purple wants to some input from transport.
 */
static void * requestInput(const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb,PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	Log().Get("purple") << "new requestInput";
	if (primary) {
		std::string primaryString(primary);
		Log().Get("purple") << "primary string: " << primaryString;
		User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
		if (!user) return NULL;
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
			data.id = "";
			user->setAdhocData(data);
		}
	}
	return NULL;
}

static void *requestFields(const char *title, const char *primary, const char *secondary, PurpleRequestFields *fields, const char *ok_text, GCallback ok_cb, const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who, PurpleConversation *conv, void *user_data) {
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
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
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
	if (user && !user->adhocData().id.empty()) {
		AdhocRepeater *repeater = new AdhocRepeater(GlooxMessageHandler::instance(), user, title ? std::string(title) : std::string(), primary ? std::string(primary) : std::string(), secondary ? std::string(secondary) : std::string(), default_action, user_data, action_count, actions);
		GlooxMessageHandler::instance()->adhoc()->registerSession(user->adhocData().from, repeater);
		AdhocData data;
		data.id = "";
		user->setAdhocData(data);
		return repeater;
	}
	else if (title) {
		std::string headerString(title);
		Log().Get("purple") << "header string: " << headerString;
		if (headerString == "SSL Certificate Verification") {
			va_arg(actions, char *);
			((PurpleRequestActionCb) va_arg(actions, GCallback)) (user_data, 2);
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
static void newXfer(PurpleXfer *xfer) {
	Log().Get("purple filetransfer") << "new file transfer request from legacy network";
	GlooxMessageHandler::instance()->purpleFileReceiveRequest(xfer);
}

/*
 * Called when file from legacy network is completely received.
 */
static void XferComplete(PurpleXfer *xfer) {
	Log().Get("purple filetransfer") << "filetransfer complete";
	GlooxMessageHandler::instance()->purpleFileReceiveComplete(xfer);
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

	// User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(account);
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

static void XferCreated(PurpleXfer *xfer) {
	std::string remote_user(purple_xfer_get_remote_user(xfer));
	
	std::for_each( remote_user.begin(), remote_user.end(), replaceBadJidCharacters() );
	
	Log().Get("xfercreated") << "get user " << remote_user;
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (!user) return;

	FiletransferRepeater *repeater = user->getFiletransfer(remote_user);
	Log().Get(user->jid()) << "get filetransferRepeater" << remote_user;
	if (repeater) {
		Log().Get(user->jid()) << "registerXfer";
		repeater->registerXfer(xfer);
	}
	else {
		std::string name(xfer->who);
		std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
		size_t pos = name.find("/");
		if (pos != std::string::npos)
			name.erase((int) pos, name.length() - (int) pos);
		user->addFiletransfer(name + "@" + GlooxMessageHandler::instance()->jid() + "/bot");
		FiletransferRepeater *repeater = user->getFiletransfer(name);
		repeater->registerXfer(xfer);
	}
}

static void fileSendStart(PurpleXfer *xfer) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->fileSendStart();
	Log().Get("filesend") << "fileSendStart()";
}

static void fileRecvStart(PurpleXfer *xfer) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->fileRecvStart();
	Log().Get("filesend") << "fileRecvStart()";
}

static void buddyListSaveNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(a);
	if (!user) return;
	if (user->loadingBuddiesFromDB()) return;
	user->storeBuddy(buddy);

}

static void buddyListNewNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	buddy->node.ui_data = NULL;
	GlooxMessageHandler::instance()->purpleBuddyCreated(buddy);
}

static void buddyListRemoveNode(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	PurpleBuddy *buddy = (PurpleBuddy *) node;
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByAccount(a);
	if (user != NULL) {
		if (user->loadingBuddiesFromDB()) return;
		long id = 0;
		if (buddy->node.ui_data) {
			long *p = (long *) buddy->node.ui_data;
			id = *p;
		}
		std::string name(purple_buddy_get_name(buddy));
		GlooxMessageHandler::instance()->sql()->removeBuddy(user->storageId(), name, id);
	}
}

static void buddyListSaveAccount(PurpleAccount *account) {
}

static gssize XferWrite(PurpleXfer *xfer, const guchar *buffer, gssize size) {
	std::cout << "gotData\n";
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	std::string d((char *) buffer, size);
	if (repeater->getResender())
		repeater->getResender()->getMutex()->lock();
	repeater->gotData(d);
	if (repeater->getResender())
		repeater->getResender()->getMutex()->unlock();
	return size;
}

static gssize XferRead(PurpleXfer *xfer, guchar **buffer, gssize size) {
	Log().Get("REPEATER") << "xferRead";
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	if (!repeater->getResender()) {
		repeater->wantsData();
		return 0;
	}
	repeater->getResender()->getMutex()->lock();
	if (repeater->getBuffer().empty()) {
		Log().Get("REPEATER") << "buffer is empty, setting wantsData = true";
		repeater->wantsData();
		repeater->getResender()->getMutex()->unlock();
		return 0;
	}
	else {
		std::string data;
		if ((gssize) repeater->getBuffer().size() > size) {
			data = repeater->getBuffer().substr(0, size);
			repeater->getBuffer().erase(0, size);
		}
		else {
			data = repeater->getBuffer();
			repeater->getBuffer().erase();
		}
		(*buffer) = (guchar *) g_malloc0(data.size());
		memcpy((*buffer), data.c_str(), data.size());
		repeater->getResender()->getMutex()->unlock();
		return data.size();
	}
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
	buddyListNewNode,
	NULL,
	NULL, // buddyListUpdate,
	NULL,
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

static PurpleXferUiOps xferUiOps =
{
	XferCreated,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	XferWrite,
	XferRead,
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
 * Connect user to legacy network
 */
static gboolean connectUser(gpointer data) {
	std::string name((char*)data);
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
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
	User *user = GlooxMessageHandler::instance()->userManager()->getUserByJID(name);
	if (user) {
		user->connect();
	}
	g_free(data);
	return FALSE;
}

/*
 * Checking new connections for our gloox proxy...
 */
static gboolean ftServerReceive(gpointer data){
	if (GlooxMessageHandler::instance()->ftServer->recv(1) == ConnNoError)
		return TRUE;
	return FALSE;
}

static gboolean transportReconnect(gpointer data) {
	GlooxMessageHandler::instance()->transportConnect();
	return FALSE;
}

/*
 * Called by notifier when new data can be received from socket
 */
static gboolean transportDataReceived(GIOChannel *source, GIOCondition condition, gpointer data) {
	GlooxMessageHandler::instance()->j->recv(1000);
	return TRUE;
}

GlooxMessageHandler::GlooxMessageHandler(const std::string &config) : MessageHandler(),ConnectionListener(),PresenceHandler(),SubscriptionHandler() {
	m_pInstance = this;
	m_firstConnection = true;
	ftManager = NULL;
	ft = NULL;
	lastIP = 0;
	capsCache["_default"] = 0;
	m_parser = NULL;
	m_sql = NULL;
	m_collector = NULL;
	m_searchHandler = NULL;
	m_reg = NULL;
	m_adhoc = NULL;
	gatewayHandler = NULL;
	ftServer = NULL;
	m_stats = NULL;

	bool loaded = true;

	if (!loadConfigFile(config))
		loaded = false;

	if (loaded && !nodaemon)
		daemonize();

#ifndef WIN32
	if (logfile || !nodaemon) {
		if (logfile) {
			std::string l(logfile);
			replace(l, "$jid", m_configuration.jid.c_str());
			logfile = g_strdup(l.c_str());
			g_mkdir_with_parents(g_path_get_dirname(logfile), 0755);
		}
		int logfd = open(logfile ? logfile : "/dev/null", O_WRONLY | O_CREAT, 0644);
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

	if (loaded) {
		m_sql = new SQLClass(this, upgrade_db);	
		if (!m_sql->loaded())
			loaded = false;
	}

	j = new HiComponent("jabber:component:accept",m_configuration.server,m_configuration.jid,m_configuration.password,m_configuration.port);

	j->logInstance().registerLogHandler(LogLevelDebug, LogAreaXmlIncoming | LogAreaXmlOutgoing, this);
	
	m_loop = g_main_loop_new(NULL, FALSE);

	m_userManager = new UserManager(this);
	m_searchHandler = NULL;

	if (loaded && !initPurple())
		loaded = false;
	
	if (loaded && !loadProtocol())
		loaded = false;

	if (loaded) {

		m_discoHandler = new GlooxDiscoHandler(this);

		m_discoInfoHandler = new GlooxDiscoInfoHandler(this);
		j->registerIqHandler(m_discoInfoHandler,ExtDiscoInfo);

		m_adhoc = new GlooxAdhocHandler(this);
		m_parser = new GlooxParser();
		m_collector = new AccountCollector();

		ftManager = new FileTransferManager();
		ft = new SIProfileFT(j, ftManager);
		ftManager->setSIProfileFT(ft, this);
		ftServer = new SOCKS5BytestreamServer(j->logInstance(), 8000, configuration().bindIPs[0]);
		if( ftServer->listen() != ConnNoError ) {}
		ft->addStreamHost( j->jid(), configuration().bindIPs[0], 8000 );
		ft->registerSOCKS5BytestreamServer( ftServer );
		purple_timeout_add(10, &ftServerReceive, NULL);
		// ft->addStreamHost(gloox::JID("proxy.jabbim.cz"), "88.86.102.51", 7777);

		j->registerMessageHandler(this);
		j->registerConnectionListener(this);
		gatewayHandler = new GlooxGatewayHandler(this);
		j->registerIqHandler(gatewayHandler, ExtGateway);
		m_reg = new GlooxRegisterHandler(this);
		j->registerIqHandler(m_reg, ExtRegistration);
		m_stats = new GlooxStatsHandler(this);
		j->registerIqHandler(m_stats, ExtStats);
		m_vcard = new GlooxVCardHandler(this);
		j->registerIqHandler(m_vcard, ExtVCard);
		j->registerPresenceHandler(this);
		j->registerSubscriptionHandler(this);

		transportConnect();

		g_main_loop_run(m_loop);
	}
}

GlooxMessageHandler::~GlooxMessageHandler(){
	purple_core_quit();
	g_main_loop_quit(m_loop);
	g_main_loop_unref(m_loop);
	delete m_userManager;
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
	// TODO: there are timers in commented classes, so wa have to stop them before purple_core_quit();
// 	if (ft)
// 		delete ft;
// 	if (ftServer)
// 		delete ftServer;
	if (m_protocol)
		delete m_protocol;
	if (m_searchHandler)
		delete m_searchHandler;
	delete j;
}

bool GlooxMessageHandler::loadProtocol(){
	if (configuration().protocol == "aim")
		m_protocol = (AbstractProtocol*) new AIMProtocol(this);
	else if (configuration().protocol == "facebook")
		m_protocol = (AbstractProtocol*) new FacebookProtocol(this);
	else if (configuration().protocol == "gg")
		m_protocol = (AbstractProtocol*) new GGProtocol(this);
	else if (configuration().protocol == "icq")
		m_protocol = (AbstractProtocol*) new ICQProtocol(this);
	else if (configuration().protocol == "irc")
		m_protocol = (AbstractProtocol*) new IRCProtocol(this);
	else if (configuration().protocol == "msn")
		m_protocol = (AbstractProtocol*) new MSNProtocol(this);
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
	else {
		Log().Get("loadProtocol") << "Protocol \"" << configuration().protocol << "\" does not exist.";
		Log().Get("loadProtocol") << "Protocol has to be one of: facebook, gg, msn, irc, xmpp, myspace, qq, simple, aim, yahoo.";
		return false;
	}

	if (!purple_find_prpl(m_protocol->protocol().c_str())) {
		Log().Get("loadProtocol") << "There is no libpurple plugin installed for protocol \"" << configuration().protocol << "\"";
		return false;
	}

	if (!m_protocol->userSearchAction().empty()) {
		m_searchHandler = new GlooxSearchHandler(this);
		j->registerIqHandler(m_searchHandler, ExtSearch);
	}
	return true;
}

void GlooxMessageHandler::handleLog(LogLevel level, LogArea area, const std::string &message) {
// 	if (m_configuration.logAreas & LOG_AREA_XML) {
		if (area == LogAreaXmlIncoming)
			Log().Get("XML IN") << message;
		else
			Log().Get("XML OUT") << message;
// 	}
}

void GlooxMessageHandler::onSessionCreateError(SessionCreateError error) {
	Log().Get("gloox") << "sessionCreateError";
}

void GlooxMessageHandler::purpleConnectionError(PurpleConnection *gc,PurpleConnectionError reason,const char *text) {
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = userManager()->getUserByAccount(account);
	if (user != NULL) {
		Log().Get(user->jid()) << "Disconnected from legacy network because of error " << int(reason);
		if (text)
			Log().Get(user->jid()) << std::string(text);
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
				purple_timeout_add_seconds(5, &reconnect, g_strdup(user->jid().c_str()));
			}
		}
	}
}

void GlooxMessageHandler::purpleBuddyTyping(PurpleAccount *account, const char *who){
	User *user = userManager()->getUserByAccount(account);
	if (user != NULL) {
		user->purpleBuddyTyping((std::string)who);
	}
	else {
		Log().Get("purple") << "purpleBuddyTyping called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::purpleBuddyRemoved(PurpleBuddy *buddy) {
	if (buddy != NULL) {
		PurpleAccount *a = purple_buddy_get_account(buddy);
		User *user = userManager()->getUserByAccount(a);
		if (user != NULL)
			user->purpleBuddyRemoved(buddy);
	}
}

void GlooxMessageHandler::purpleBuddyCreated(PurpleBuddy *buddy) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = userManager()->getUserByAccount(a);
	if (user != NULL)
		user->purpleBuddyCreated(buddy);
}

void GlooxMessageHandler::purpleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = userManager()->getUserByAccount(a);
	if (user != NULL)
		user->purpleBuddyStatusChanged(buddy, status, old_status);
}

void GlooxMessageHandler::purpleBuddySignedOn(PurpleBuddy *buddy) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = userManager()->getUserByAccount(a);
	if (user != NULL)
		user->purpleBuddySignedOn(buddy);
}

void GlooxMessageHandler::purpleBuddySignedOff(PurpleBuddy *buddy) {
	PurpleAccount *a = purple_buddy_get_account(buddy);
	User *user = userManager()->getUserByAccount(a);
	if (user != NULL)
		user->purpleBuddySignedOff(buddy);
}

void GlooxMessageHandler::purpleBuddyTypingStopped(PurpleAccount *account, const char *who) {
	User *user = userManager()->getUserByAccount(account);
	if (user != NULL) {
		user->purpleBuddyTypingStopped((std::string) who);
	}
	else {
		Log().Get("purple") << "purpleBuddyTypingStopped called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::signedOn(PurpleConnection *gc, gpointer unused) {
	PurpleAccount *account = purple_connection_get_account(gc);
	User *user = userManager()->getUserByAccount(account);
	if (user != NULL) {
		Log().Get(user->jid()) << "logged in to legacy network";
		user->connected();
	}
}

void GlooxMessageHandler::purpleAuthorizeClose(void *data) {
	authData *d = (authData*) data;
	if (!d)
		return;
	User *user = userManager()->getUserByAccount(d->account);
	if (user != NULL) {
		Log().Get(user->jid()) << "purple wants to close authorizeRequest";
		if (user->hasAuthRequest(d->who)) {
			Log().Get(user->jid()) << "closing authorizeRequest";
			user->removeAuthRequest(d->who);
		}
	}
	else {
		Log().Get("purple") << "purpleAuthorizationClose called, but user does not exist!!!";
	}
}

void * GlooxMessageHandler::purpleAuthorizeReceived(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data){
	if (account==NULL)
		return NULL;
	User *user = userManager()->getUserByAccount(account);
	if (user != NULL) {
		if (user->isConnected()) {
			user->purpleAuthorizeReceived(account, remote_user, id, alias, message, on_list, authorize_cb, deny_cb, user_data);
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
}

/*
 * Load config file and parses it and save result to m_configuration
 */
bool GlooxMessageHandler::loadConfigFile(const std::string &config) {
	GKeyFile *keyfile;
	int flags;
  	char **bind;
	char *value;
	int i;

	keyfile = g_key_file_new ();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

	if (!g_key_file_load_from_file (keyfile, config.c_str(), (GKeyFileFlags)flags, NULL)) {
		if (!g_key_file_load_from_file (keyfile, std::string("/etc/spectrum/" + config + ".cfg").c_str(), (GKeyFileFlags)flags, NULL))
		{
			Log().Get("loadConfigFile") << "Can't load config file!";
			Log().Get("loadConfigFile") << std::string("/etc/spectrum/" + config + ".cfg") << " or ./" << config;

			g_key_file_free(keyfile);
			return false;
		}
	}

	if ((value = g_key_file_get_string(keyfile, "service","protocol", NULL)) != NULL) {
		m_configuration.protocol = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `protocol` in [service] part of config file.";
		return false;
	}

	if ((value = g_key_file_get_string(keyfile, "service","name", NULL)) != NULL) {
		m_configuration.discoName = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `name` in [service] part of config file.";
		return false;
	}

	if ((value = g_key_file_get_string(keyfile, "service","server", NULL)) != NULL) {
		m_configuration.server = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `server` in [service] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "service","password", NULL)) != NULL) {
		m_configuration.password = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `password` in [service] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "service","jid", NULL)) != NULL) {
		m_configuration.jid = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `jid` in [service] part of config file.";
		return false;
	}
	
	if (g_key_file_get_integer(keyfile, "service","port", NULL))
		m_configuration.port = (int)g_key_file_get_integer(keyfile, "service","port", NULL);
	else {
		Log().Get("loadConfigFile") << "You have to specify `port` in [service] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "service","filetransfer_cache", NULL)) != NULL) {
		m_configuration.filetransferCache = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `filetransfer_cache` in [service] part of config file.";
		return false;
	}

	if (lock_file == NULL) {
		if ((value = g_key_file_get_string(keyfile, "service","pid_file", NULL)) != NULL) {
			std::string pid_f(value);
			replace(pid_f, "$jid", m_configuration.jid.c_str());
			g_mkdir_with_parents(g_path_get_dirname(pid_f.c_str()), 0755);
			lock_file = g_strdup(pid_f.c_str());
		}
		else {
			lock_file = g_strdup(std::string("/var/run/spectrum/" + m_configuration.jid).c_str());
			g_mkdir_with_parents("/var/run/spectrum", 0755);
		}
	}

	if ((value = g_key_file_get_string(keyfile, "database","type", NULL)) != NULL) {
		m_configuration.sqlType = std::string(value);
		g_free(value);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `type` in [database] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "database","host", NULL)) != NULL) {
		m_configuration.sqlHost = std::string(value);
		g_free(value);
	}
	else if (m_configuration.sqlType == "sqlite")
		m_configuration.sqlHost = "";
	else {
		Log().Get("loadConfigFile") << "You have to specify `host` in [database] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "database","password", NULL)) != NULL) {
		m_configuration.sqlPassword = std::string(value);
		g_free(value);
	}
	else if (m_configuration.sqlType == "sqlite")
		m_configuration.sqlPassword = "";
	else {
		Log().Get("loadConfigFile") << "You have to specify `password` in [database] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "database","user", NULL)) != NULL) {
		m_configuration.sqlUser = std::string(value);
		g_free(value);
	}
	else if (m_configuration.sqlType == "sqlite")
		m_configuration.sqlUser = "";
	else {
		Log().Get("loadConfigFile") << "You have to specify `user` in [database] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "database","database", NULL)) != NULL) {
		m_configuration.sqlDb = std::string(value);
		g_free(value);
		replace(m_configuration.sqlDb, "$jid", m_configuration.jid.c_str());
		g_mkdir_with_parents(g_path_get_dirname(m_configuration.sqlDb.c_str()), 0755);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `database` in [database] part of config file.";
		return false;
	}
	
	if ((value = g_key_file_get_string(keyfile, "database","prefix", NULL)) != NULL) {
		m_configuration.sqlPrefix = std::string(value);
		g_free(value);
	}
	else if (m_configuration.sqlType == "sqlite")
		m_configuration.sqlPrefix = "";
	else {
		Log().Get("loadConfigFile") << "You have to specify `prefix` in [database] part of config file.";
		return false;
	}

	if ((value = g_key_file_get_string(keyfile, "purple","userdir", NULL)) != NULL) {
		m_configuration.userDir = std::string(value);
		g_free(value);
		replace(m_configuration.userDir, "$jid", m_configuration.jid.c_str());
		g_mkdir_with_parents(g_path_get_dirname(m_configuration.userDir.c_str()), 0755);
	}
	else {
		Log().Get("loadConfigFile") << "You have to specify `userdir` in [purple] part of config file.";
		return false;
	}
		

	if (g_key_file_has_key(keyfile,"service","language",NULL))
		m_configuration.language = g_key_file_get_string(keyfile, "service","language", NULL);
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
	else m_configuration.transportFeatures = TRANSPORT_FEATURE_AVATARS | TRANSPORT_FEATURE_FILETRANSFER | TRANSPORT_FEATURE_TYPING_NOTIFY;

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
	else m_configuration.VIPFeatures = TRANSPORT_FEATURE_AVATARS | TRANSPORT_FEATURE_FILETRANSFER | TRANSPORT_FEATURE_TYPING_NOTIFY;

	if(g_key_file_has_key(keyfile,"service","use_proxy",NULL))
		m_configuration.useProxy=g_key_file_get_boolean(keyfile, "service","use_proxy", NULL);
	else
		m_configuration.useProxy = false;

	if (g_key_file_has_key(keyfile,"logging","log_file",NULL)) {
		logfile = g_key_file_get_string(keyfile, "logging","log_file", NULL);
	}
	
	if (g_key_file_has_key(keyfile,"logging","log_areas",NULL)) {
		bind = g_key_file_get_string_list (keyfile,"logging","log_areas", NULL, NULL);
		m_configuration.logAreas = 0;
		for (i = 0; bind[i]; i++) {
			std::string feature(bind[i]);
			if (feature == "xml") {
				m_configuration.logAreas = m_configuration.logAreas | LOG_AREA_XML;
			}
			else if (feature == "purple")
				m_configuration.logAreas = m_configuration.logAreas | LOG_AREA_PURPLE;
		}
		g_strfreev (bind);
	}
	else m_configuration.logAreas = LOG_AREA_XML | LOG_AREA_PURPLE;
	
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

	if(g_key_file_has_key(keyfile,"service","admins",NULL)) {
		bind = g_key_file_get_string_list(keyfile, "service", "admins", NULL, NULL);
		for (i = 0; bind[i]; i++){
			m_configuration.admins.push_back((std::string) bind[i]);
		}
		g_strfreev (bind);
	}

	g_key_file_free(keyfile);
	return true;
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

	User *user = userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (user != NULL) {
		FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
		if (user->hasFeature(GLOOX_FEATURE_FILETRANSFER)) {
			repeater->requestFT();
		}
		else {
			purple_xfer_request_accepted(xfer, std::string(configuration().filetransferCache+"/"+remote_user+"-"+j->getID()+"-"+filename).c_str());
		}
	}
}

void GlooxMessageHandler::purpleFileReceiveComplete(PurpleXfer *xfer) {
	std::string filename(purple_xfer_get_filename(xfer));
	std::string remote_user(purple_xfer_get_remote_user(xfer));
	if (purple_xfer_get_local_filename(xfer)) {
		std::string localname(purple_xfer_get_local_filename(xfer));
		std::string basename(g_path_get_basename(purple_xfer_get_local_filename(xfer)));
		User *user = userManager()->getUserByAccount(purple_xfer_get_account(xfer));
		if (user != NULL) {
			if (user->isConnected()) {
				if (user->isVIP()) {
					sql()->addDownload(basename,"1");
				}
				else {
					sql()->addDownload(basename,"0");
				}
				Message s(Message::Chat, user->jid(), tr(user->getLang(),_("File '"))+filename+tr(user->getLang(), _("' was received. You can download it here: ")) + "http://soumar.jabbim.cz/icq/" + basename +" .");
				s.setFrom(remote_user + "@" + jid() + "/bot");
				j->send(s);
			}
			else{
				Log().Get(user->jid()) << "purpleFileReceiveComplete called for unconnected user...";
			}
		}
		else
			Log().Get("purple") << "purpleFileReceiveComplete called, but user does not exist!!!";
	}
}


void GlooxMessageHandler::purpleMessageReceived(PurpleAccount* account, char * name, char *msg, PurpleConversation *conv, PurpleMessageFlags flags) {
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			user->purpleMessageReceived(account,name,msg,conv,flags);
		}
		else {
			Log().Get(user->jid()) << "purpleMessageReceived called for unconnected user...";
		}
	}
	else {
		Log().Get("purple") << "purpleMessageReceived called, but user does not exist!!!";
	}
}

void GlooxMessageHandler::purpleConversationWriteIM(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			m_stats->messageFromLegacy();
			user->purpleConversationWriteIM(conv, who, message, flags, mtime);
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
			s.setFrom(protocol()->notifyUsername() + "@" + jid() + "/bot");
			j->send(s);
		}
	}
}

void GlooxMessageHandler::purpleConversationWriteChat(PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL)
		return;
	PurpleAccount *account = purple_conversation_get_account(conv);
	User *user = userManager()->getUserByAccount(account);
	if (user) {
		if (user->isConnected()) {
			m_stats->messageFromLegacy();
			user->purpleConversationWriteChat(conv, who, message, flags, mtime);
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
		if (user->isConnected()) {
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
		if (user->isConnected()) {
			user->purpleChatRemoveUsers(conv, users);
		}
	}
}

void GlooxMessageHandler::purpleBuddyChanged(PurpleBuddy* buddy) {
	if (buddy != NULL) {
		PurpleAccount *a = purple_buddy_get_account(buddy);
		User *user = userManager()->getUserByAccount(a);
		if (user != NULL)
			user->purpleBuddyChanged(buddy);
	}
}

bool GlooxMessageHandler::hasCaps(const std::string &ver) {
	std::map<std::string,int> ::iterator iter = capsCache.begin();
	iter = capsCache.find(ver);
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

	if (stanza.presence() != Presence::Unavailable && ((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || protocol()->isMUC(NULL, stanza.to().bare()))) {
		Tag *stanzaTag = stanza.tag();
		if (!stanzaTag) return;
		Tag *c = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/caps");
		// presence has caps
		if (c != NULL) {
			// caps is not chached
			if (!hasCaps(c->findAttribute("ver"))) {
				// ask for caps
				std::string id = j->getID();
				Log().Get(stanza.from().full()) << "asking for caps with ID: " << id;
				m_discoHandler->versions[m_discoHandler->version].version = c->findAttribute("ver");
				m_discoHandler->versions[m_discoHandler->version].jid = stanza.to().full();
				std::string node;
				node = c->findAttribute("node") + std::string("#") + c->findAttribute("ver");
				j->disco()->getDiscoInfo(stanza.from(), node, m_discoHandler, m_discoHandler->version, id);
				m_discoHandler->version++;
			}
			else {
				std::string id = j->getID();
				Log().Get(stanza.from().full()) << "asking for disco#info with ID: " << id;
				m_discoHandler->versions[m_discoHandler->version].version = stanza.from().full();
				m_discoHandler->versions[m_discoHandler->version].jid = stanza.to().full();
				j->disco()->getDiscoInfo(stanza.from(), "", m_discoHandler, m_discoHandler->version, id);
				m_discoHandler->version++;
			}
		}
		else {
			std::string id = j->getID();
			Log().Get(stanza.from().full()) << "asking for disco#info with ID: " << id;
			m_discoHandler->versions[m_discoHandler->version].version = stanza.from().full();
			m_discoHandler->versions[m_discoHandler->version].jid = stanza.to().full();
			j->disco()->getDiscoInfo(stanza.from(), "", m_discoHandler, m_discoHandler->version, id);
			m_discoHandler->version++;
		}
		delete stanzaTag;
	}
	User *user;
	std::string userkey;
	if (protocol()->isMUC(NULL, stanza.to().bare())) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		userkey = stanza.from().bare() + server;
		user = userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = userManager()->getUserByJID(stanza.from().bare());
		userkey = stanza.from().bare();
	}
	if (user == NULL) {
		// we are not connected and probe arrived => answer with unavailable
		if (stanza.subtype() == Presence::Probe) {
			Log().Get(stanza.from().full()) << "Answering to probe presence with unavailable presence";
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", stanza.from().full());
			tag->addAttribute("from", stanza.to().bare() + "/bot");
			tag->addAttribute("type", "unavailable");
			j->send(tag);
		}
		else if (((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || protocol()->isMUC(NULL, stanza.to().bare())) && stanza.presence() != Presence::Unavailable){
			UserRow res = sql()->getUserByJid(userkey);
			if(res.id==-1 && !protocol()->tempAccountsAllowed()) {
				// presence from unregistered user
				Log().Get(stanza.from().full()) << "This user is not registered";
				return;
			}
			else {
				if(res.id==-1 && protocol()->tempAccountsAllowed()) {
					sql()->addUser(userkey,stanza.from().username(),"","en");
					res = sql()->getUserByJid(userkey);
				}
				bool isVip = sql()->isVIP(stanza.from().bare());
				std::list<std::string> const &x = configuration().allowedServers;
				if (configuration().onlyForVIP && !isVip && std::find(x.begin(), x.end(), stanza.from().server()) == x.end()) {
					Log().Get(stanza.from().full()) << "This user is not VIP, can't login...";
					return;
				}
				Log().Get(stanza.from().full()) << "Creating new User instance";
				if (protocol()->tempAccountsAllowed()) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					std::cout << "SERVER" << stanza.from().bare() + server << "\n";
					user = new User(this, stanza.from(), stanza.to().resource() + "@" + server, "", stanza.from().bare() + server, res.id);
				}
				else {
					if (purple_accounts_find(res.uin.c_str(), protocol()->protocol().c_str()) != NULL) {
						PurpleAccount *act = purple_accounts_find(res.uin.c_str(), protocol()->protocol().c_str());
						if (userManager()->getUserByAccount(act)) {
							Log().Get(stanza.from().full()) << "This account is already connected by another jid " << user->jid();
							return;
						}
					}
					user = new User(this, stanza.from(), res.uin, res.password, stanza.from().bare(), res.id);
				}
				user->setFeatures(isVip ? configuration().VIPFeatures : configuration().transportFeatures);
				if (c != NULL)
					if (hasCaps(c->findAttribute("ver")))
						user->setResource(stanza.from().resource(), stanza.priority(), c->findAttribute("ver"));

				std::map<int,std::string> ::iterator iter = configuration().bindIPs.begin();
				iter = configuration().bindIPs.find(lastIP);
				if (iter != configuration().bindIPs.end()) {
					user->setBindIP(configuration().bindIPs[lastIP]);
					lastIP++;
				}
				else {
					lastIP = 0;
					user->setBindIP(configuration().bindIPs[lastIP]);
				}
				m_userManager->addUser(user);
				user->receivedPresence(stanza);
				if (protocol()->isMUC(NULL, stanza.to().bare())) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					server = stanza.from().bare() + server;
					purple_timeout_add_seconds(15, &connectUser, g_strdup(server.c_str()));
				}
				else
					purple_timeout_add_seconds(15, &connectUser, g_strdup(stanza.from().bare().c_str()));
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
	else {
		user->receivedPresence(stanza);
	}
	if (stanza.to().username() == "" && user != NULL) {
		if(stanza.presence() == Presence::Unavailable && user->isConnected() == true && user->resources().empty()) {
			Log().Get(stanza.from().full()) << "Logging out";
			m_userManager->removeUser(user);
		}
		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false && int(time(NULL)) > int(user->connectionStart()) + 10 && user->resources().empty()) {
			Log().Get(stanza.from().full()) << "Logging out, but he's not connected...";
			m_userManager->removeUser(user);
		}
		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false) {
			Log().Get(stanza.from().full()) << "Can't logout because we're connecting now...";
		}
	}
}

void GlooxMessageHandler::onConnect() {
	Log().Get("gloox") << "CONNECTED!";
	j->disco()->setIdentity("gateway", protocol()->gatewayIdentity(), configuration().discoName);
	j->disco()->setVersion(configuration().discoName, VERSION, "");

	std::string id = "gateway";
	id += '/';
	id += protocol()->gatewayIdentity();
	id += '/';
	id += '/';
	id += configuration().discoName;
	id += '<';
	
	std::list<std::string> features = protocol()->transportFeatures();
	features.sort();
	for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
		j->disco()->addFeature(*it);
	}

	std::list<std::string> f = protocol()->buddyFeatures();
	f.sort();
	for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
		id += (*it);
		id += '<';
	}

    SHA sha;
    sha.feed( id );
    m_configuration.hash = Base64::encode64( sha.binary() );

	if (m_firstConnection) {
		new AutoConnectLoop(this);
		m_firstConnection = false;
	}
}

void GlooxMessageHandler::onDisconnect(ConnectionError e) {
	Log().Get("gloox") << "Disconnected from Jabber server !";
	switch (e) {
		case ConnNoError: Log().Get("gloox") << "Reason: No error"; break;
		case ConnStreamError: Log().Get("gloox") << "Reason: Stream error"; break;
		case ConnStreamVersionError: Log().Get("gloox") << "Reason: Stream version error"; break;
		case ConnStreamClosed: Log().Get("gloox") << "Reason: Stream closed"; break;
		case ConnProxyAuthRequired: Log().Get("gloox") << "Reason: Proxy auth required"; break;
		case ConnProxyAuthFailed: Log().Get("gloox") << "Reason: Proxy auth failed"; break;
		case ConnProxyNoSupportedAuth: Log().Get("gloox") << "Reason: Proxy no supported auth"; break;
		case ConnIoError: Log().Get("gloox") << "Reason: IO Error"; break;
		case ConnParseError: Log().Get("gloox") << "Reason: Parse error"; break;
		case ConnConnectionRefused: Log().Get("gloox") << "Reason: Connection refused"; break;
// 		case ConnSocketError: Log().Get("gloox") << "Reason: Socket error"; break;
		case ConnDnsError: Log().Get("gloox") << "Reason: DNS Error"; break;
		case ConnOutOfMemory: Log().Get("gloox") << "Reason: Out Of Memory"; break;
		case ConnNoSupportedAuth: Log().Get("gloox") << "Reason: No supported auth"; break;
		case ConnTlsFailed: Log().Get("gloox") << "Reason: Tls failed"; break;
		case ConnTlsNotAvailable: Log().Get("gloox") << "Reason: Tls not available"; break;
		case ConnCompressionFailed: Log().Get("gloox") << "Reason: Compression failed"; break;
// 		case ConnCompressionNotAvailable: Log().Get("gloox") << "Reason: Compression not available"; break;
		case ConnAuthenticationFailed: Log().Get("gloox") << "Reason: Authentication Failed"; break;
		case ConnUserDisconnected: Log().Get("gloox") << "Reason: User disconnected"; break;
		case ConnNotConnected: Log().Get("gloox") << "Reason: Not connected"; break;
	};

	switch (j->streamError()) {
		case StreamErrorBadFormat: Log().Get("gloox") << "Stream error: Bad format"; break;
		case StreamErrorBadNamespacePrefix: Log().Get("gloox") << "Stream error: Bad namespace prefix"; break;
		case StreamErrorConflict: Log().Get("gloox") << "Stream error: Conflict"; break;
		case StreamErrorConnectionTimeout: Log().Get("gloox") << "Stream error: Connection timeout"; break;
		case StreamErrorHostGone: Log().Get("gloox") << "Stream error: Host gone"; break;
		case StreamErrorHostUnknown: Log().Get("gloox") << "Stream error: Host unknown"; break;
		case StreamErrorImproperAddressing: Log().Get("gloox") << "Stream error: Improper addressing"; break;
		case StreamErrorInternalServerError: Log().Get("gloox") << "Stream error: Internal server error"; break;
		case StreamErrorInvalidFrom: Log().Get("gloox") << "Stream error: Invalid from"; break;
		case StreamErrorInvalidId: Log().Get("gloox") << "Stream error: Invalid ID"; break;
		case StreamErrorInvalidNamespace: Log().Get("gloox") << "Stream error: Invalid Namespace"; break;
		case StreamErrorInvalidXml: Log().Get("gloox") << "Stream error: Invalid XML"; break;
		case StreamErrorNotAuthorized: Log().Get("gloox") << "Stream error: Not Authorized"; break;
		case StreamErrorPolicyViolation: Log().Get("gloox") << "Stream error: Policy violation"; break;
		case StreamErrorRemoteConnectionFailed: Log().Get("gloox") << "Stream error: Remote connection failed"; break;
		case StreamErrorResourceConstraint: Log().Get("gloox") << "Stream error: Resource constraint"; break;
		case StreamErrorRestrictedXml: Log().Get("gloox") << "Stream error: Restricted XML"; break;
		case StreamErrorSeeOtherHost: Log().Get("gloox") << "Stream error: See other host"; break;
		case StreamErrorSystemShutdown: Log().Get("gloox") << "Stream error: System shutdown"; break;
		case StreamErrorUndefinedCondition: Log().Get("gloox") << "Stream error: Undefined Condition"; break;
		case StreamErrorUnsupportedEncoding: Log().Get("gloox") << "Stream error: Unsupported encoding"; break;
		case StreamErrorUnsupportedStanzaType: Log().Get("gloox") << "Stream error: Unsupported stanza type"; break;
		case StreamErrorUnsupportedVersion: Log().Get("gloox") << "Stream error: Unsupported version"; break;
		case StreamErrorXmlNotWellFormed: Log().Get("gloox") << "Stream error: XML Not well formed"; break;
		case StreamErrorUndefined: Log().Get("gloox") << "Stream error: Error undefined"; break;
	};

	switch (j->authError()) {
		case AuthErrorUndefined: Log().Get("gloox") << "Auth error: Error undefined"; break;
		case SaslAborted: Log().Get("gloox") << "Auth error: Sasl aborted"; break;
		case SaslIncorrectEncoding: Log().Get("gloox") << "Auth error: Sasl incorrect encoding"; break;        
		case SaslInvalidAuthzid: Log().Get("gloox") << "Auth error: Sasl invalid authzid"; break;
		case SaslInvalidMechanism: Log().Get("gloox") << "Auth error: Sasl invalid mechanism"; break;
		case SaslMalformedRequest: Log().Get("gloox") << "Auth error: Sasl malformed request"; break;
		case SaslMechanismTooWeak: Log().Get("gloox") << "Auth error: Sasl mechanism too weak"; break;
		case SaslNotAuthorized: Log().Get("gloox") << "Auth error: Sasl Not authorized"; break;
		case SaslTemporaryAuthFailure: Log().Get("gloox") << "Auth error: Sasl temporary auth failure"; break;
		case NonSaslConflict: Log().Get("gloox") << "Auth error: Non sasl conflict"; break;
		case NonSaslNotAcceptable: Log().Get("gloox") << "Auth error: Non sasl not acceptable"; break;
		case NonSaslNotAuthorized: Log().Get("gloox") << "Auth error: Non sasl not authorized"; break;
	};

	if (j->streamError() == 0 || j->streamError() == 24) {
		Log().Get("gloox") << "trying to reconnect after 3 seconds";
		purple_timeout_add_seconds(3, &transportReconnect, NULL);
	}
}

void GlooxMessageHandler::transportConnect() {
	j->connect(false);
	int mysock = dynamic_cast<ConnectionTCPClient*>( j->connectionImpl() )->socket();
	if (mysock > 0) {
		connectIO = g_io_channel_unix_new(mysock);
		g_io_add_watch(connectIO, (GIOCondition) READ_COND, &transportDataReceived, NULL);
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
	if (protocol()->isMUC(NULL, msg.to().bare())) {
		std::string server = msg.to().username().substr(msg.to().username().find("%") + 1, msg.to().username().length() - msg.to().username().find("%"));
		user = userManager()->getUserByJID(msg.from().bare() + server);
	}
	else {
		user = userManager()->getUserByJID(msg.from().bare());
	}
	if (user!=NULL) {
		if (user->isConnected()) {
			Tag *msgTag = msg.tag();
			if (!msgTag) return;
			Tag *chatstates = msgTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/chatstates");
			if (chatstates != NULL) {
				std::string username = msg.to().username();
				std::for_each( username.begin(), username.end(), replaceJidCharacters() );
				user->receivedChatState(username, chatstates->name());
			}
			if (msgTag->findChild("body") != NULL) {
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

	purple_util_set_user_dir(configuration().userDir.c_str());

	if (m_configuration.logAreas & LOG_AREA_PURPLE)
		purple_debug_set_enabled(true);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

	ret = purple_core_init(PURPLE_UI);
	if (ret) {
		static int conversation_handle;
		static int conn_handle;
		static int xfer_handle;
		static int blist_handle;

		purple_set_blist(purple_blist_new());
		purple_blist_load();

		purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &conversation_handle, PURPLE_CALLBACK(newMessageReceived), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-send-start", &xfer_handle, PURPLE_CALLBACK(fileSendStart), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-recv-start", &xfer_handle, PURPLE_CALLBACK(fileRecvStart), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-recv-request", &xfer_handle, PURPLE_CALLBACK(newXfer), NULL);
		purple_signal_connect(purple_xfers_get_handle(), "file-recv-complete", &xfer_handle, PURPLE_CALLBACK(XferComplete), NULL);
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
// 	g_timeout_add(0, &deleteMain, NULL);
	delete GlooxMessageHandler::instance();

	return;
}

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

	if (argc != 2)
#if WIN32
		std::cout << "Usage: spectrum.exe <configuration_file.cfg>";
#else
		std::cout << g_option_context_get_help(context, FALSE, NULL);
#endif
	else {

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

		std::string config(argv[1]);
		new GlooxMessageHandler(config);
	}
	g_option_context_free(context);
}

