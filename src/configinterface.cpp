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

#include "configinterface.h"
#include "sys/un.h"
#include "sys/socket.h"
#include "log.h"
#include "errno.h"
#include "transport.h"
#include "abstractconfiginterfacehandler.h"


// because of ConnectionBase::socket()....
static int getUnixSocket() {
	return socket(AF_UNIX, SOCK_STREAM, 0);
}

static void gotData(gpointer data, gint source, PurpleInputCondition cond) {
	ConfigInterface *interface = (ConfigInterface *) data;
	interface->recv();
}

static void gotClientData(gpointer data, gint source, PurpleInputCondition cond) {
	ConnectionBase *connection = (ConnectionBase *) data;
	connection->recv();
}

static void dataParsed(Tag *tag, void *data) {
	ConfigInterface *interface = (ConfigInterface *) data;
	if (tag)
		interface->handleTag(tag);
}

ConfigInterface::ConfigInterface(const std::string &sockfile, const LogSink &logInstance) : ConnectionTCPServer(this, logInstance, "localhost", -1) {
	// Create UNIX socket
	int m_socket;
	socklen_t length;
	struct sockaddr_un local;
	m_loaded = false;
	m_socketId = 0;
	
	if ((m_socket = getUnixSocket()) == -1)
		Log("ConfigInterface", "Could not create UNIX socket: " << strerror(errno));
	
	local.sun_family = AF_UNIX;
	
	strcpy(local.sun_path, sockfile.c_str());
	unlink(local.sun_path);
	
	length = offsetof(struct sockaddr_un, sun_path) + strlen(sockfile.c_str());

	if (bind(m_socket, (struct sockaddr *) &local, length) == -1) {
		Log("ConfigInterface", "Could not bind to UNIX socket: " << sockfile << " " << strerror(errno));
		return;
	}
	
	if (listen(m_socket, 5) == -1) {
		Log("ConfigInterface", "Could not listen to UNIX socket: " << sockfile << " " << strerror(errno));
		return;
	}

	setSocket(m_socket);
	m_socketId = purple_input_add(m_socket, PURPLE_INPUT_READ, gotData, this);
	m_loaded = true;
}

ConfigInterface::~ConfigInterface() {
	if (m_socketId)
		g_source_remove(m_socketId);
}

void ConfigInterface::handleIncomingConnection(ConnectionBase *server, ConnectionBase *connection) {
	if (!m_loaded)
		return;
	Log("ConfigInterface", "Incomming connection " << connection);
	connection->registerConnectionDataHandler(this);
	ConnectionTCPBase *tcpBase = (ConnectionTCPBase *) connection;
	m_clients[connection] = purple_input_add(tcpBase->socket(), PURPLE_INPUT_READ, gotClientData, connection);
}

void ConfigInterface::handleReceivedData( const ConnectionBase* connection, const std::string& data ) {
	if (!m_loaded)
		return;
	Log("ConfigInterface", "[XML IN] " << data);
	std::map <ConnectionBase *, guint>::iterator it = m_clients.find( const_cast<ConnectionBase*>( connection ) );
	m_connection = (*it).first;
	Transport::instance()->parser()->getTag(data, dataParsed, this);
}

void ConfigInterface::handleConnect( const ConnectionBase* connection ) {}

void ConfigInterface::handleDisconnect( const ConnectionBase* connection, ConnectionError reason ) {
	if (!m_loaded)
		return;
	Log("ConfigInterface", "Disconnect connection " << connection);
	g_source_remove(m_clients[const_cast<ConnectionBase*>( connection )]);
	m_clients.erase(const_cast<ConnectionBase*>( connection ));
	delete connection;
}

void ConfigInterface::handleTag(Tag *tag) {
	for (std::list <AbstractConfigInterfaceHandler *>::const_iterator i = m_handlers.begin(); i != m_handlers.end(); i++) {
		if ((*i)->handleCondition(tag)) {
			Tag *response = (*i)->handleTag(tag);
			if (response) {
				m_connection->send(response->xml());
				delete response;
				break;
			}
		}
	}
	delete tag;
}

void ConfigInterface::registerHandler(AbstractConfigInterfaceHandler *handler) {
	m_handlers.push_back(handler);
}
