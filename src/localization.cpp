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
#include <locale.h>
#include <libintl.h>
#include <stdlib.h>

Localization::Localization() {
    // remove the LANGUAGE environment variable, to keep from
    // messing up gettext(3)
    unsetenv("LANGUAGE");
    unsetenv("LANG");
    unsetenv("LC_MESSAGES");

    // set the domain for message files.
    textdomain("spectrum");

    // create a local cache for the locale strings,
    // hopefully allowing a bit of performance improvement when
    // running as a daemon..
    m_locales = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
					(GDestroyNotify)g_hash_table_destroy);
}

Localization::~Localization() {
    g_hash_table_destroy(m_locales);
}

const char * Localization::translate(const char *lang, const char *key) {
    const char *ret = key;
    GHashTable *locale;

    if (!(locale = (GHashTable*) g_hash_table_lookup(m_locales, lang))) {
	// didn't find the locale, so lets create it and put it
	// in the locales hash table.
	locale = g_hash_table_new_full(g_str_hash,
					    g_str_equal, g_free, g_free);

	g_hash_table_replace(m_locales, g_strdup(lang), locale);
    }
    if (locale) {
	char *col = g_utf8_collate_key(key, -1);
	if (!(ret = (const char *) g_hash_table_lookup(locale, col))) {
	    // we didn't find the message in the cache, so lets
	    // look it up using gettext()
	    setlocale(LC_MESSAGES, lang);

	    // ask gettext for our translation
	    if (!(ret = (const char *)gettext(key))) {
		ret = key;
	    } else {
		gchar *value = g_strdup(ret);
		gchar *nkey = g_strdup(key);
		g_hash_table_replace(locale, nkey, value);
	    }
	}

	g_free(col);
    }

    return ret;
}

Localization localization;
