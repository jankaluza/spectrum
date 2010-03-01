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

#ifndef CONFIG_INTERFACE_H
#define CONFIG_INTERFACE_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include "gloox/presence.h"
#include "gloox/connectiontcpserver.h"
#include "gloox/connectionhandler.h"
#include "gloox/connectiondatahandler.h"

extern LogClass Log_;

using namespace gloox;

class AbstractConfigInterfaceHandler;

class ConfigInterface : public ConnectionTCPServer, public ConnectionHandler, public ConnectionDataHandler {
	public:
		ConfigInterface(const std::string &filename, const LogSink &logInstance);
		virtual ~ConfigInterface();

		// ConnectionHandler
		void handleIncomingConnection(ConnectionBase *server, ConnectionBase *connection);

		// ConnectionDataHandler
		void handleReceivedData( const ConnectionBase* connection, const std::string& data );
		void handleConnect( const ConnectionBase* connection );
		void handleDisconnect( const ConnectionBase* connection, ConnectionError reason );

		void handleTag(Tag *tag);
		void registerHandler(AbstractConfigInterfaceHandler *handler);


	private:
		int m_socket;
		guint m_socketId;
		std::map <ConnectionBase *, guint> m_clients;
		ConnectionBase *m_connection;
		std::list <AbstractConfigInterfaceHandler *> m_handlers;
		bool m_loaded;

};

#endif
