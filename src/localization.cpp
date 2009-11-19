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
}

Localization::~Localization() {
}

const char * Localization::translate(const char *lang, const char *key) {
	const char *ret = key;

	setlocale(LC_MESSAGES, lang);

	// ask gettext for our translation
	if (!(ret = gettext(key)))
	    ret = key ;

	return ret;
}

Localization localization;
