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
#include <iostream>

SQLClass::SQLClass(const std::string &config) {
	m_sess = NULL;
	
	loadConfigFile(config);
	
	try {
		MySQL::Connector::registerConnector(); 
		m_sess = new Session("MySQL", "user=" + m_configuration.sqlUser + ";password=" + m_configuration.sqlPassword + ";host=" + m_configuration.sqlHost + ";db=" + m_configuration.sqlDb + ";auto-reconnect=true");
	}
	catch (Poco::Exception e) {
		std::cout << e.displayText() << "\n";
		return;
	}
	
	if (!m_sess)
		return;
	try {
		std::cout << "Deleting old temp tables\n";
		*m_sess << "DROP TABLE IF EXISTS buddies_new", now;
		*m_sess << "DROP TABLE IF EXISTS buddies_settings_new", now;
		*m_sess << "DROP TABLE IF EXISTS users_new", now;
		*m_sess << "DROP TABLE IF EXISTS users_settings_new", now;
		
		std::cout << "Creating new temp tables\n";
		*m_sess << "CREATE TABLE `buddies_new` (\n"
			"`id` int(10) unsigned NOT NULL auto_increment,\n"
			"`user_id` int(10) unsigned NOT NULL,\n"
			"`uin` varchar(255) collate utf8_bin NOT NULL,\n"
			"`subscription` enum('to','from','both','ask','none') collate utf8_bin NOT NULL,\n"
			"`nickname` varchar(255) collate utf8_bin NOT NULL,\n"
			"`groups` varchar(255) collate utf8_bin NOT NULL,\n"
			"PRIMARY KEY (`id`),\n"
			"UNIQUE KEY `user_id` (`user_id`,`uin`)\n"
		") ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n", now;
		*m_sess << "CREATE TABLE IF NOT EXISTS `buddies_settings_new` (\n"
			"`user_id` int(10) unsigned NOT NULL,\n"
			"`buddy_id` int(10) unsigned NOT NULL,\n"
			"`var` varchar(50) collate utf8_bin NOT NULL,\n"
			"`type` smallint(4) unsigned NOT NULL,\n"
			"`value` varchar(255) collate utf8_bin NOT NULL,\n"
			"PRIMARY KEY (`buddy_id`,`var`),\n"
			"KEY `buddy_id` (`buddy_id`),\n"
			"KEY `user_id` (`user_id`)\n"
		") ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n", now;
		*m_sess << "CREATE TABLE IF NOT EXISTS `users_new` (\n"
			"`id` int(10) unsigned NOT NULL auto_increment,\n"
			"`jid` varchar(255) collate utf8_bin NOT NULL,\n"
			"`uin` varchar(4095) collate utf8_bin NOT NULL,\n"
			"`password` varchar(255) collate utf8_bin NOT NULL,\n"
			"`language` varchar(25) collate utf8_bin NOT NULL,\n"
			"`encoding` varchar(50) collate utf8_bin NOT NULL default 'utf8',\n"
			"`last_login` datetime,\n"
			"`vip` tinyint(1) NOT NULL default '0',\n"
			"PRIMARY KEY (`id`),\n"
			"UNIQUE KEY `jid` (`jid`)\n"
		") ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n", now;
		*m_sess << "CREATE TABLE IF NOT EXISTS `users_settings_new` (\n"
			"`user_id` int(10) unsigned NOT NULL,\n"
			"`var` varchar(50) collate utf8_bin NOT NULL,\n"
			"`type` smallint(4) unsigned NOT NULL,\n"
			"`value` varchar(255) collate utf8_bin NOT NULL,\n"
			"PRIMARY KEY (`user_id`,`var`),\n"
			"KEY `user_id` (`user_id`)\n"
		") ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin;", now;
		
		std::cout << "Migrating data from `" + m_configuration.sqlPrefix + "users` table to temporary table\n";
		*m_sess << std::string("INSERT users_new SELECT * FROM " + m_configuration.sqlPrefix + "users;"), now;
	
		std::cout << "Migrating data from `" + m_configuration.sqlPrefix + "rosters` table to temporary table\n";
		*m_sess << "INSERT INTO buddies_new (`id`,  `user_id`,  `uin`,  `subscription`,  `nickname`,  `groups`) SELECT AA.`id`,  AB.`id`,  AA.`uin`,  AA.`subscription`,  AA.`nickname`,  AA.`g` FROM " + m_configuration.sqlPrefix + "rosters AA, users_new AB WHERE AA.`jid`=AB.`jid`";

		std::cout << "Migrating data from `" + m_configuration.sqlPrefix + "settings` table to temporary table\n";
		*m_sess << "INSERT INTO users_settings_new (`user_id`,  `var`,  `type`,  `value`) SELECT AB.`id`,  AA.`var`,  AA.`type`,  AA.`value` FROM " + m_configuration.sqlPrefix + "settings AA, users_new AB WHERE AA.`jid`=AB.`jid`";
	}
		catch (Poco::Exception e) {
		std::cout << "\n" << e.displayText() << "\n";
		return;
	}
	m_sess->close();
}


bool SQLClass::loadConfigFile(const std::string &config) {
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
			std::cout << "Can't load config file!\n";
			std::cout << std::string("/etc/spectrum/" + config + ".cfg") << " or ./" << config << "\n";

			g_key_file_free(keyfile);
			return false;
		}
	}

	value = g_key_file_get_string(keyfile, "service","protocol", NULL);
	m_configuration.protocol = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "service","name", NULL);
	m_configuration.discoName = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "service","server", NULL);
	m_configuration.server = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "service","password", NULL);
	m_configuration.password = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "service","jid", NULL);
	m_configuration.jid = std::string(value);
	g_free(value);
	
	m_configuration.port = (int)g_key_file_get_integer(keyfile, "service","port", NULL);
	
	value = g_key_file_get_string(keyfile, "service","filetransfer_cache", NULL);
	m_configuration.filetransferCache = std::string(value);
	g_free(value);

	value = g_key_file_get_string(keyfile, "database","type", NULL);
	m_configuration.sqlType = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "database","host", NULL);
	m_configuration.sqlHost = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "database","password", NULL);
	m_configuration.sqlPassword = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "database","user", NULL);
	m_configuration.sqlUser = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "database","database", NULL);
	m_configuration.sqlDb = std::string(value);
	g_free(value);
	
	value = g_key_file_get_string(keyfile, "database","prefix", NULL);
	m_configuration.sqlPrefix = std::string(value);
	g_free(value);

	g_key_file_free(keyfile);
	return true;
}

int main( int argc, char* argv[] ) {
	if (argc != 2) {
		std::cout << "Converts mysql database from old pre-release model to new model.\n";
		std::cout << "Usage: migratedb spectrum_config_file.cfg.\n";
	}
	else {
		std::string config(argv[1]);
		SQLClass MigrateClass(config);
	}
}