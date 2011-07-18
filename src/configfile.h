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
#include <list>
#include <map>
#include <string>
#include "limits.h"

struct PurpleAccountSettingValue {
	char *str;
	int i;
	bool b;
	std::list <std::string> strlist;
	PurplePrefType type;
};

// Structure used for storing transport configuration.
struct Configuration {
	std::string discoName;			// Name for service discovery.
	std::string protocol;			// Libpurple protocol.
	std::string server;				// XMPP server address.
	std::string password;			// Password to connect server.
	std::string jid;				// Spectrum JID.
	std::string encoding;			// Default encoding.
	std::string sqlVIP;
	int port;						// Server port.
	std::string config_interface;	// ConfigInterface address.
	
	int logAreas;					// Logging areas.
	std::string logfile;			// Logging file.
	std::string pid_f;				// File to store PID.

	bool onlyForVIP;				// True if transport is only for VIP.
	bool VIPEnabled;				// True if VIP mode is enabled.
	int transportFeatures;			// Transport features for all users.
	int VIPFeatures;				// Transport features for VIP users.
	std::string language;			// Default language.
	bool useProxy;					// True if transport has to use proxy to connect legacy network.
	bool jid_escaping;				// True if jid escaping is enabled by default.
	bool require_tls;
	bool filetransferForceDir;
	std::string filetransfer_proxy_ip;
	int filetransfer_proxy_port;
	std::string filetransfer_proxy_streamhost_ip;
	int filetransfer_proxy_streamhost_port;

	bool enable_public_registration;
	std::string username_mask;
	std::string reg_instructions;
	std::string reg_username_field;
	std::string reg_allowed_usernames;
	std::list <std::string> reg_extra_fields;

	std::string userDir;			// Directory used to store avatars.
	std::string filetransferCache;	// Directory where transfered files are stored.
	std::string filetransferWeb;
	std::string eventloop;

	std::string sqlHost;			// Database host.
	std::string sqlPassword;		// Database password.
	std::string sqlUser;			// Database user.
	std::string sqlDb;				// Database name.
	std::string sqlPrefix;			// Prefix for database tables.
	std::string sqlType;			// Type of database.
	std::string sqlCryptKey;
	
	std::string hash; 				// Version hash used for caps.
	
	std::list <std::string> allowedServers;		// Hostnames which are allowed to connect.
	std::list <std::string> admins;				// JIDs of Spectrum administrators
	std::map<int,std::string> bindIPs;			// IP addresses used to bind connections.
	std::map<std::string, PurpleAccountSettingValue> purple_account_settings;

	operator bool() const {
		return !protocol.empty();
	}
};

// Loads Spectrum config file, parses it and stores into Configuration structure.
class ConfigFile {
	public:
		// If filename is provided, loadFromFile is called automatically.
		ConfigFile(const std::string &filename = "");
		~ConfigFile();

		// Loads configuration from files.
		void loadFromFile(const std::string &file);

		// Loads configuration from data.
		void loadFromData(const std::string &data);

		// Returns parsed configuration.
		Configuration getConfiguration();

		// Loads PurpleAcccountSettings. This has to be called after libpurple initialization.
		void loadPurpleAccountSettings(Configuration &configuration);

#ifndef TESTS
	private:
#endif
		GKeyFile *keyfile;
		bool m_loaded;
		std::string m_jid;
		std::string m_protocol;
		std::string m_appdata;
		std::string m_filename;
		std::string m_transport;
		int m_port;

		bool loadString(std::string &variable, const std::string &section, const std::string &key, const std::string &def = "required");
		bool loadInteger(int &variable, const std::string &section, const std::string &key, int def = INT_MAX);
		bool loadBoolean(bool &variable, const std::string &section, const std::string &key, bool def = false, bool required = false);
		bool loadStringList(std::list <std::string> &variable, const std::string &section, const std::string &key);
		bool loadHostPort(std::string &variable, int &port, const std::string &section, const std::string &key, const std::string &def_host = "required", const int &def_port = 0);
		bool loadFeatures(int &features, const std::string &section);
};

#endif
