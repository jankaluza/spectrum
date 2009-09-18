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

Localization::Localization() {
	m_locales = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
}

Localization::~Localization() {
	g_hash_table_destroy(m_locales);
}

const char * Localization::translate(const char *lang, const char *key) {
	const char *ret = key;
	GHashTable *l;
	if (!(l = (GHashTable*) g_hash_table_lookup(m_locales, lang))) {
		loadLocale(lang);
		l = (GHashTable*) g_hash_table_lookup(m_locales, lang);
	}
	if (l) {
		char *col = g_utf8_collate_key(key, -1);
		if (!(ret = (const char *) g_hash_table_lookup(l, col))) {
			ret = key;
		}
		g_free(col);
	}
	return ret;
}

bool Localization::loadLocale(const std::string &lang) {
#ifndef WIN32
	po_file_t pofile = NULL;

#if LIBGETTEXTPO_VERSION < 0x000E07
	po_error_handler_t error_handle;
#else
	po_xerror_handler_t error_handle;
#endif
	const char * const *domains;
	const char * const *domainp;
	const char *msgstr = NULL;
	const char *msgid = NULL;
#endif
	// Already loaded
	if (g_hash_table_lookup(m_locales, lang.c_str()))
		return true;
	g_hash_table_replace(m_locales, g_strdup(lang.c_str()), g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free));
#ifndef WIN32
	char *l = g_build_filename(INSTALL_DIR, "share", "spectrum", "locales", std::string(lang + ".po").c_str(), NULL);
	pofile = po_file_read (l, error_handle);
	g_free(l);
	if (pofile != NULL) {
		GHashTable *locale = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		Log().Get("Localization") << lang  << " locale found";
		domains = po_file_domains (pofile);
		for (domainp = domains; *domainp; domainp++) {
			const char *domain = *domainp;
			po_message_t message;
			po_message_iterator_t iterator = po_message_iterator (pofile,
									      domain);
			message = po_next_message (iterator);
			for (;message;) {
				gchar *key;
				gchar *value;
				msgstr = po_message_msgstr (message);
				msgid = po_message_msgid (message);
				if (msgstr && msgid) {
					if (g_utf8_collate (msgid, "") != 0) {
						key = g_utf8_collate_key (msgid, -1);
						value = g_strdup (msgstr);
						g_hash_table_replace(locale, key, value);
					}
				}
				message = po_next_message (iterator);
			}
			po_message_iterator_free (iterator);
		}
		g_hash_table_replace(m_locales, g_strdup(lang.c_str()), locale);
		po_file_free(pofile);
		return true;
	}
	#endif
	return false;
}

Localization localization;
