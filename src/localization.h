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
#ifndef _HI_LOCALIZATION_HANDLER_H
#define _HI_LOCALIZATION_HANDLER_H

#include <string>
#include <map>
#include "glib.h"
#include "purple.h"
#include <iostream>
#include <fstream>

class MoFile {
	public:
		MoFile(const std::string &filename);
		~MoFile();

		const char *lookup(const char *s);

		bool isLoaded();
		bool isReversed() { return m_reversed; }
		char *getData() { return m_data; }

	private:
		std::ifstream::pos_type m_size;
		char *m_data;
		bool m_reversed;
		int m_num_strings;
		int m_original_table_offset;
		int m_translated_table_offset;
		int m_hash_num_entries;
		int m_hash_offset;
};

class Translation {
	public:
		Translation(const std::string &lang);
		~Translation();

		const char * translate(const char *key);

	private:
		MoFile *m_spectrum;
		MoFile *m_pidgin;
};

class Localization {
	public:
		Localization();
		~Localization();

		bool loadLocale(const std::string &lang);
		const char * translate(const char *lang, const char *key);
		const char * translate(const char *lang, const std::string &key) { return translate(lang, key.c_str()); }
		const char * translate(const std::string &lang, const std::string &key) { return translate(lang.c_str(), key.c_str()); }
		std::map <std::string, std::string> &getLanguages();

	private:
		GHashTable *m_locales;		// xml_lang, hash table with localizations
		std::map<std::string, std::string> m_languages;
};

#endif
