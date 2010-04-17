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

#include "localization.h"
#include "log.h"
#include "transport_config.h"
#include "string.h"

#define HASHWORDBITS 32

inline unsigned long hashpjw(const char *str_param) {
	unsigned long hval = 0;
	unsigned long g;
	const char *s = str_param;

	while (*s) {
		hval <<= 4;
		hval += (unsigned char) *s++;
		g = hval & ((unsigned long int) 0xf << (HASHWORDBITS - 4));
		if (g != 0) {
			hval ^= g >> (HASHWORDBITS - 8);
			hval ^= g;
		}
	}

	return hval;
}


// Swap the endianness of a 4-byte word.
// On some architectures you can replace my_swap4 with an inlined instruction.
inline unsigned long my_swap4(unsigned long result) {
	unsigned long c0 = (result >> 0) & 0xff;
	unsigned long c1 = (result >> 8) & 0xff;
	unsigned long c2 = (result >> 16) & 0xff;
	unsigned long c3 = (result >> 24) & 0xff;

	return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
}

inline int read4_from_offset(MoFile *mo_file, int offset) {
	unsigned long *ptr = (unsigned long *)(((char *)mo_file->getData()) + offset);
	
	if (mo_file->isReversed()) {
		return my_swap4(*ptr);
	} else {
		return *ptr;
	}
}

MoFile::MoFile(const std::string &filename) {
	m_data = NULL;
	m_size = 0;
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open()) {
		m_size = file.tellg();
		m_data = new char [m_size];
		file.seekg (0, std::ios::beg);
		file.read (m_data, m_size);
		file.close();
	}
	
	if (m_data) {
		unsigned long *long_ptr = (unsigned long *) m_data;

		const unsigned long TARGET_MAGIC = 0x950412DE;
		const unsigned long TARGET_MAGIC_REVERSED = 0xDE120495;
		unsigned long magic = long_ptr[0];

		if (magic == TARGET_MAGIC) {
			m_reversed = false;
		} else if (magic == TARGET_MAGIC_REVERSED) {
			m_reversed = true;
		} else {
			delete[] m_data;
			m_data = NULL;
			return;
		}

		m_num_strings = read4_from_offset(this, 8);
		m_original_table_offset = read4_from_offset(this, 12);
		m_translated_table_offset = read4_from_offset(this, 16);
		m_hash_num_entries = read4_from_offset(this, 20);
		m_hash_offset = read4_from_offset(this, 24);
		
		if (m_hash_num_entries == 0) {
			delete[] m_data;
			m_data = NULL;
			return;
		}
	}
}

MoFile::~MoFile() {
	if (m_data)
		delete[] m_data;
}

bool MoFile::isLoaded() {
	return m_data != NULL;
}

const char *MoFile::lookup(const char *s) {
	if (m_data == NULL)
		return s;

	unsigned long V = hashpjw(s);
	int S = m_hash_num_entries;

	int hash_cursor = V % S;
	int orig_hash_cursor = hash_cursor;
	int increment = 1 + (V % (S - 2));
	int target_index = -1;
	
	while (1) {
		unsigned int index = read4_from_offset(this, m_hash_offset + 4 * hash_cursor);
		if (index == 0) break;

		index--;  // Because entries in the table are stored +1 so that 0 means empty.

		int addr_offset = m_original_table_offset + 8 * index + 4;
		int string_offset = read4_from_offset(this, addr_offset);

		char *t = ((char *)m_data) + string_offset;

		if (strcmp(s, t) == 0) {
			target_index = index;
			break;
		}

		hash_cursor += increment;
		hash_cursor %= S;

		if (hash_cursor == orig_hash_cursor) break;
	}
	
	if (target_index == -1) return s;  // Maybe we want to log an error?

    int addr_offset = m_translated_table_offset + 8 * target_index + 4;
    int string_offset = read4_from_offset(this, addr_offset);

    char *t = ((char *)m_data) + string_offset;
		
	return t;
}

static void deleteTranslation(gpointer data) {
	Translation *trans = (Translation *) data;
	delete trans;
}


Translation::Translation(const std::string &lang) {
	char *l;
	l = g_build_filename(INSTALL_DIR, "share", "locale", lang.c_str(), "LC_MESSAGES", "spectrum.mo", NULL);
	m_spectrum = new MoFile(l);
	g_free(l);

	l = g_build_filename(INSTALL_DIR, "share", "locale", lang.c_str(), "LC_MESSAGES", "pidgin.mo", NULL);
	m_pidgin = new MoFile(l);
	g_free(l);
}

Translation::~Translation() {
	delete m_pidgin;
	delete m_spectrum;
}

const char * Translation::translate(const char *key) {
	const char *ret = m_spectrum->lookup(key);
	if (strcmp(ret, key) == 0)
		return m_pidgin->lookup(key);
	return ret;
}

Localization::Localization() {
	m_locales = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, deleteTranslation);
}

Localization::~Localization() {
	g_hash_table_destroy(m_locales);
}

const char * Localization::translate(const char *lang, const char *key) {
	Translation *trans;
	if (!(trans = (Translation *) g_hash_table_lookup(m_locales, lang))) {
		loadLocale(lang);
		trans = (Translation *) g_hash_table_lookup(m_locales, lang);
	}
	if (trans) {
		return trans->translate(key);
	}
	return key;
}

bool Localization::loadLocale(const std::string &lang) {
	// Already loaded
	if (g_hash_table_lookup(m_locales, lang.c_str()))
		return true;
#ifndef WIN32
	Translation *trans = new Translation(lang);
	g_hash_table_replace(m_locales, g_strdup(lang.c_str()), trans);
#endif
	return false;
}

Localization localization;
