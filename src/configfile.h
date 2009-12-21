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

struct Configuration;

class ConfigFile {
	public:
		ConfigFile(const std::string &filename);
		~ConfigFile();
		Configuration getConfiguration();
		
	
	private:
		GKeyFile *keyfile;
		bool m_loaded;
		std::string m_jid;
		std::string m_protocol;

		bool loadString(std::string &variable, const std::string &section, const std::string &key, const std::string &def = "required");
		bool loadInteger(int &variable, const std::string &section, const std::string &key, int def = INT_MAX);
		bool loadBoolean(bool &variable, const std::string &section, const std::string &key, bool def, bool required = false);
};

#endif