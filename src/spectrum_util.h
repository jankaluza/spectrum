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

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <cstdio>
#include <iostream>
#include "glib.h"

template <class T> std::string stringOf(T object) {
	std::ostringstream os;
	os << object;
	return (os.str());
}

/* Replace all instances of from with to in string str in-place */
void replace(std::string &str, const char *from, const char *to);
int isValidEmail(const char *address);
const std::string generateUUID();

#ifndef WIN32
void process_mem_usage(double& vm_usage, double& resident_set);
#endif

#endif
