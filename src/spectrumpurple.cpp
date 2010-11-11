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

#include "spectrumpurple.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Network/BoostIOServiceThread.h"
#include "spectrumeventloop.h"
#include "geventloop.h"
#include "log.h"
#include "spectrum_util.h"
#include "transport.h"
#include "main.h"

static GIOChannel *channel;

/*

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
}*/

static void printDebug(PurpleDebugLevel level, const char *category, const char *arg_s) {
	std::string c("libpurple");

	if (category) {
		c.push_back('/');
		c.append(category);
	}

// 	LogMessage(Log_.fileStream(), false).Get(c) << arg_s;
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

static PurpleCoreUiOps coreUiOps =
{
	NULL,
	/*debug_init*/NULL,
	/*transport_core_ui_init*/NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void signed_on(PurpleConnection *gc, gpointer unused) {
	PurpleAccount *account = purple_connection_get_account(gc);
	std::cout << "SIGNED ON\n";
	SpectrumBackendMessage msg(MSG_CONNECTED, (char *) account->ui_data);
	msg.send(channel);
}

static void do_login(const char *protocol, const char *uin, const char *passwd, int status, const char *msg) {
	PurpleAccount *account = purple_accounts_find(uin, protocol);
	if (account == NULL) {
// 		Log(m_jid, "creating new account");
		account = purple_account_new(uin, protocol);
		purple_accounts_add(account);
	}
// 	Transport::instance()->collector()->stopCollecting(account);

	PurplePlugin *plugin = purple_find_prpl(protocol);
	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
	for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
		PurpleAccountOption *option = (PurpleAccountOption *) l->data;
		purple_account_remove_setting(account, purple_account_option_get_setting(option));
	}

	std::map <std::string, PurpleAccountSettingValue> &settings = Transport::instance()->getConfiguration().purple_account_settings;
	for (std::map <std::string, PurpleAccountSettingValue>::iterator it = settings.begin(); it != settings.end(); it++) {
		PurpleAccountSettingValue v = (*it).second;
		std::string key((*it).first);
		switch (v.type) {
			case PURPLE_PREF_BOOLEAN:
				purple_account_set_bool(account, key.c_str(), v.b);
				break;

			case PURPLE_PREF_INT:
				purple_account_set_int(account, key.c_str(), v.i);
				break;

			case PURPLE_PREF_STRING:
				if (v.str)
					purple_account_set_string(account, key.c_str(), v.str);
				else
					purple_account_remove_setting(account, key.c_str());
				break;

			case PURPLE_PREF_STRING_LIST:
				// TODO:
				break;

			default:
				continue;
		}
	}

// 	purple_account_set_string(account, "encoding", m_encoding.empty() ? Transport::instance()->getConfiguration().encoding.c_str() : m_encoding.c_str());
	purple_account_set_bool(account, "use_clientlogin", false);
	purple_account_set_bool(account, "require_tls",  Transport::instance()->getConfiguration().require_tls);
	purple_account_set_bool(account, "use_ssl",  Transport::instance()->getConfiguration().require_tls);
	purple_account_set_bool(account, "direct_connect", false);
// 	purple_account_set_bool(account, "check-mail", purple_value_get_boolean(getSetting("enable_notify_email")));

	account->ui_data = g_strdup(uin);
	
	Transport::instance()->protocol()->onPurpleAccountCreated(account);

// 	m_loadingBuddiesFromDB = true;
// 	loadRoster();
// 	m_loadingBuddiesFromDB = false;

// 	m_connectionStart = time(NULL);
// 	m_readyForConnect = false;
	purple_account_set_password(account,passwd);
// 	Log(m_jid, "UIN:" << m_username << " USER_ID:" << m_userID);

	if (CONFIG().useProxy) {
		PurpleProxyInfo *info = purple_proxy_info_new();
		purple_proxy_info_set_type(info, PURPLE_PROXY_USE_ENVVAR);
		info->username = NULL;
		info->password = NULL;
		purple_account_set_proxy_info(account, info);
	}

// 	if (valid && purple_value_get_boolean(getSetting("enable_transport"))) {
		purple_account_set_enabled(account, PURPLE_UI, TRUE);
// 		purple_account_connect(account);
		const PurpleStatusType *statusType = purple_account_get_status_type_with_primitive(account, (PurpleStatusPrimitive) status);
		if (statusType) {
// 			Log(m_jid, "Setting up default status.");
			if (msg) {
				purple_account_set_status(account, purple_status_type_get_id(statusType), TRUE, "message", msg, NULL);
			}
			else {
				purple_account_set_status(account, purple_status_type_get_id(statusType), TRUE, NULL);
			}
		}
// 	}
}

static void do_change_status(const char *protocol, const char *uin, int status, const char *msg) {
	PurpleAccount *account = purple_accounts_find(uin, protocol);
	if (account) {
		const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(account, (PurpleStatusPrimitive) status);
		if (status_type != NULL) {
			if (msg) {
				purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, "message", msg, NULL);
			}
			else {
				purple_account_set_status(account, purple_status_type_get_id(status_type), TRUE, NULL);
			}
		}
	}
}

static void do_logout(const char *protocol, const char *uin) {
	PurpleAccount *account = purple_accounts_find(uin, protocol);
	if (account) {
		purple_account_set_enabled(account, PURPLE_UI, FALSE);

		// Remove conversations.
		// This has to be called before m_account->ui_data = NULL;, because it uses
		// ui_data to call SpectrumMessageHandler::purpleConversationDestroyed() callback.
		GList *iter;
		for (iter = purple_get_conversations(); iter; ) {
			PurpleConversation *conv = (PurpleConversation*) iter->data;
			iter = iter->next;
			if (purple_conversation_get_account(conv) == account)
				purple_conversation_destroy(conv);
		}

		g_free(account->ui_data);
		account->ui_data = NULL;
// 		Transport::instance()->collector()->collect(m_account);
	}
}

static gboolean dataReceived(GIOChannel *source, GIOCondition condition, gpointer d) {
	SpectrumPurple *parent = (SpectrumPurple *) d;
	gchar header[8];
	gsize bytes_read;
	size_t data_size;
	g_io_channel_read_chars(source, header, 8, &bytes_read, NULL);
	if (bytes_read != 8)
		return TRUE;
	std::istringstream is(std::string(header, 8));
	is >> std::hex >> data_size;
	gchar *data = (gchar *) g_malloc(data_size);
	g_io_channel_read_chars(source, data, data_size, &bytes_read, NULL);

	SpectrumBackendMessage msg;
	std::istringstream ss(std::string(data, data_size));
	boost::archive::binary_iarchive ia(ss);
	ia >> msg;
	std::cout << "TYPE:" << msg.type << " " << msg.int1 << "\n";
	SpectrumBackendMessage m;
	switch(msg.type) {
		case MSG_PING:
			m.type = MSG_PONG;
			m.int1 = msg.int1;
			m.send(source);
		break;
		case MSG_UUID:
			m.type = MSG_UUID;
			m.str1 = parent->getUUID();
			m.send(source);
			parent->setUUID("");
		break;
		case MSG_LOGIN:
			do_login(msg.str4.c_str(), msg.str1.c_str(), msg.str2.c_str(), msg.int1, msg.str3.c_str());
		break;
		case MSG_CHANGE_STATUS:
			do_change_status(msg.str1.c_str(), msg.str2.c_str(), msg.int1, msg.str3.c_str());
		break;
		case MSG_LOGOUT:
			do_logout(msg.str1.c_str(), msg.str2.c_str());
		break;
	}

	g_free(data);
	return TRUE;
}

static int socket_connect(const char *host, in_port_t port) {
	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;     

	if((hp = gethostbyname(host)) == NULL){
// 			herror("gethostbyname");
// 			exit(1);
	}
	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
	if(sock == -1){
// 			perror("setsockopt");
// 			exit(1);
	}
	if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
// 			perror("connect");
// 			exit(1);
	}
	return sock;
}


SpectrumPurple::SpectrumPurple(boost::asio::io_service* ioService) {
	m_uuid = generateUUID();
	m_logged = false;
	int pid = fork();
	// child
	if (pid == 0) {
		m_parent = false;
		GMainLoop *loop = g_main_loop_new(NULL, FALSE);
		initPurple();

		int mysock = socket_connect("localhost", 19899);
		GIOChannel *connectIO = g_io_channel_unix_new(mysock);
		channel = connectIO;
		g_io_channel_set_encoding(connectIO, NULL, NULL);
		int connectID = g_io_add_watch(connectIO, (GIOCondition) READ_COND, &dataReceived, this);

		g_main_loop_run(loop);
		g_source_remove(connectID);
	}
	// parent
	else {
		m_parent = true;
	}
}

SpectrumPurple::~SpectrumPurple() {
	
}

void SpectrumPurple::initPurple() {
// 	purple_util_set_user_dir(configuration().userDir.c_str());

// 	if (m_configuration.logAreas & LOG_AREA_PURPLE)
		purple_debug_set_ui_ops(&debugUiOps);

	purple_core_set_ui_ops(&coreUiOps);
	purple_eventloop_set_ui_ops(getEventLoopUiOps());

	int ret = purple_core_init("spectrum");
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


// 		purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &conversation_handle, PURPLE_CALLBACK(newMessageReceived), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", &conversation_handle, PURPLE_CALLBACK(buddyTyping), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "buddy-typed", &conversation_handle, PURPLE_CALLBACK(buddyTyped), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped", &conversation_handle, PURPLE_CALLBACK(buddyTypingStopped), NULL);
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-removed", &blist_handle,PURPLE_CALLBACK(buddyRemoved), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &blist_handle,PURPLE_CALLBACK(buddySignedOn), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &blist_handle,PURPLE_CALLBACK(buddySignedOff), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &blist_handle,PURPLE_CALLBACK(buddyStatusChanged), NULL);
// 		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", &blist_handle,PURPLE_CALLBACK(NodeRemoved), NULL);
// 		purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed", &conversation_handle, PURPLE_CALLBACK(conv_chat_topic_changed), NULL);
// 
// 		purple_commands_init();

	}
}

void SpectrumPurple::handleConnected(bool error) {
	std::cout << "connected; error=" << error << "\n";
}

void SpectrumPurple::changeStatus(const std::string &uin, int status, const std::string &message) {
	SpectrumBackendMessage msg(MSG_CHANGE_STATUS);
	msg.str1 = Transport::instance()->protocol()->protocol();
	msg.str2 = uin;
	msg.str3 = message;
	msg.int1 = status;
	m_conn->async_write(msg, boost::bind(&SpectrumPurple::handleInstanceWrite, this, boost::asio::placeholders::error, m_conn));
}

void SpectrumPurple::login(const std::string &uin, const std::string &passwd, int status, const std::string &message) {
	SpectrumBackendMessage msg(MSG_LOGIN);
	msg.str1 = uin;
	msg.str2 = passwd;
	msg.str3 = message;
	msg.str4 = Transport::instance()->protocol()->protocol();
	msg.int1 = status;
	m_conn->async_write(msg, boost::bind(&SpectrumPurple::handleInstanceWrite, this, boost::asio::placeholders::error, m_conn));
}

void SpectrumPurple::logout(const std::string &uin) {
	SpectrumBackendMessage msg(MSG_LOGOUT);
	msg.str1 = Transport::instance()->protocol()->protocol();
	msg.str2 = uin;
	m_conn->async_write(msg, boost::bind(&SpectrumPurple::handleInstanceWrite, this, boost::asio::placeholders::error, m_conn));
}

