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

#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/SQLite/Connector.h" 
#include "Poco/Data/MySQL/Connector.h"
#include "glib.h"

#include <string.h>
#include <unistd.h>

using namespace Poco::Data;


/*
 * Struct used for storing transport configuration.
 */
struct Configuration {
	std::string discoName;	// name which will be shown in service discovery
	std::string protocol;	// protocol used for transporting
	std::string server;		// address of server to bind to
	std::string password;	// server password
	std::string jid;		// JID of this transport
	int port;				// server port

	bool onlyForVIP;		// true if transport is only for users in VIP users database
	bool VIPEnabled;
	int transportFeatures;
	std::string language;
	int VIPFeatures;
	bool useProxy;
	std::list <std::string> allowedServers;
	std::map<int,std::string> bindIPs;	// IP address to which libpurple should bind connections

	std::string userDir;	// directory used as .tmp directory for avatars and other libpurple stuff
	std::string filetransferCache;	// directory where files are saved
	std::string base64Dir;	// TODO: I'm depracted, remove me

	std::string sqlHost;	// database host
	std::string sqlPassword;	// database password
	std::string sqlUser;	// database user
	std::string sqlDb;		// database database
	std::string sqlPrefix;	// database prefix used for tables
	std::string sqlType;	// database type
};

class SQLClass {
	public:
		SQLClass(const std::string &config);
		~SQLClass() { delete m_sess; }
		
	private:
		bool loadConfigFile(const std::string &config);
		
		Poco::Data::Session *m_sess;
		Configuration m_configuration;				// configuration struct
};