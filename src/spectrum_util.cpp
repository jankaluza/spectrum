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

#include "spectrum_util.h"

#include <cstring>

int isValidEmail(const char *address) {
	int        count = 0;
	const char *c, *domain;
	static const char *rfc822_specials = "()<>@,;:\\\"[]";

	/* first we validate the name portion (name@domain) */
	for (c = address;  *c;  c++) {
		if (*c == '\"' && (c == address || *(c - 1) == '.' || *(c - 1) == 
			'\"')) {
		while (*++c) {
			if (*c == '\"') break;
			if (*c == '\\' && (*++c == ' ')) continue;
			if (*c < ' ' || *c >= 127) return 0;
		}
		if (!*c++) return 0;
		if (*c == '@') break;
		if (*c != '.') return 0;
		continue;
		}
		if (*c == '@') break;
		if (*c <= ' ' || *c >= 127) return 0;
		if (strchr(rfc822_specials, *c)) return 0;
	}
	if (c == address || *(c - 1) == '.') return 0;

	/* next we validate the domain portion (name@domain) */
	if (!*(domain = ++c)) return 0;
	do {
		if (*c == '.') {
		if (c == domain || *(c - 1) == '.') return 0;
		count++;
		}
		if (*c <= ' ' || *c >= 127) return 0;
		if (strchr(rfc822_specials, *c)) return 0;
	} while (*++c);

	return (count >= 1);
}

void replace(std::string &str, const char *from, const char *to)
{
	const size_t from_len = strlen(from);
	const size_t to_len   = strlen(to);

	// Find the first string to replace
	int index = str.find(from);

	// while there is one
	while(index != (int) std::string::npos)
	{
		// Replace it
		str.replace(index, from_len, to);
		// Find the next one
		index = str.find(from, index + to_len);
	}
}

const std::string generateUUID() {
	int tmp = g_random_int();
	int a = 0x4000 | (tmp & 0xFFF); /* 0x4000 to 0x4FFF */
	tmp >>= 12;
	int b = ((1 << 3) << 12) | (tmp & 0x3FFF); /* 0x8000 to 0xBFFF */

	tmp = g_random_int();
	char *uuid = g_strdup_printf("%08x-%04x-%04x-%04x-%04x%08x",
			g_random_int(),
			tmp & 0xFFFF,
			a, b,
			(tmp >> 16) & 0xFFFF, g_random_int());
	std::string ret(uuid);
	g_free(uuid);
	return ret;
}
