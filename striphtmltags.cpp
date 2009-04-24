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

#include "striphtmltags.h"

std::string& replaceAll(std::string& context, const std::string& from, const std::string& to) {
	size_t lookHere = 0;
	size_t foundHere;
	while((foundHere = context.find(from, lookHere)) != std::string::npos) {
		context.replace(foundHere, from.size(), to);
		lookHere = foundHere + to.size();
	}
	return context;
}

std::string& stripHTMLTags(std::string& s) {
	static bool inTag = false;
	bool done = false;

	std::string::iterator input;
	std::string::iterator output;

	input = output = s.begin();

	while (input != s.end()) {
		if (inTag) {
			inTag = !(*input++ == '>');
		} else {
			if (*input == '<') {
				inTag = true;
				input++;
			} else {
				*output++ = *input++;
			}
		}
	}
	s.erase(output, s.end());

	// Remove all special HTML characters
	replaceAll(s, "&lt;", "<");
	replaceAll(s, "&gt;", ">");
	replaceAll(s, "&amp;", "&");
	replaceAll(s, "&apos;", "'");
	replaceAll(s, "&nbsp;", " ");

    return s;
}
