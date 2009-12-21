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

#ifndef _SPECTRUM_CONFIG_FILE_H
#define _SPECTRUM_CONFIG_FILE_H

#include "glib.h"
#include "purple.h"
#include <iostream>
#include "limits.h"

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
	
	int logAreas;			// logging areas
	std::string logfile;
	std::string pid_f;

	bool onlyForVIP;		// true if transport is only for users in VIP users database
	bool VIPEnabled;
	int transportFeatures;
	std::string language;
	int VIPFeatures;
	bool useProxy;
	std::list <std::string> allowedServers;
	std::list <std::string> admins;
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
	
	std::string hash; 		// version hash used for caps
	
	operator bool() const {
		return !protocol.empty();
	}
};

class ConfigFile {
	public:
		ConfigFile(const std::string &filename);
		~ConfigFile();
		
		void loadFromFile(const std::string &file);
		void loadFromData(const std::string &data);
		
		Configuration getConfiguration();

#ifndef TESTS
	private:
#endif
		GKeyFile *keyfile;
		bool m_loaded;
		std::string m_jid;
		std::string m_protocol;

		bool loadString(std::string &variable, const std::string &section, const std::string &key, const std::string &def = "required");
		bool loadInteger(int &variable, const std::string &section, const std::string &key, int def = INT_MAX);
		bool loadBoolean(bool &variable, const std::string &section, const std::string &key, bool def = false, bool required = false);
};

#endif