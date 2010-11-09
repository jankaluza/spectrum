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

static gboolean dataReceived(GIOChannel *source, GIOCondition condition, gpointer unused) {
// 	SpectrumPurple *parent = (SpectrumPurple *) data;
	unsigned long header;
	gsize bytes_read;
	g_io_channel_read_chars(source, (gchar *) &header, sizeof(unsigned long), &bytes_read, NULL);
// 	std::cout << "HEADER:" << header << "\n";
	gchar *data = (gchar *) g_malloc(header);
	g_io_channel_read_chars(source, data, header, &bytes_read, NULL);
// 	std::cout << "DATA:" << data[3] << "\n";

	SpectrumBackendMessage msg;
	std::istringstream ss(std::string(data, header));
	boost::archive::binary_iarchive ia(ss);
	ia >> msg;
	std::cout << "TYPE:" << msg.type << " " << msg.int1 << "\n";
	switch(msg.type) {
		case PING:
			SpectrumBackendMessage m(PONG, msg.int1);
			m.send(source);
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
	int pid = fork();
	// child
	if (pid == 0) {
		m_parent = false;
		GMainLoop *loop = g_main_loop_new(NULL, FALSE);
		initPurple();

		int mysock = socket_connect("localhost", 19899);
		GIOChannel *connectIO = g_io_channel_unix_new(mysock);
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
// 		purple_signal_connect(purple_connections_get_handle(), "signed-on", &conn_handle,PURPLE_CALLBACK(signed_on), NULL);
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



