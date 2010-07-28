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
#include <vector>

#define ELAPSED_TIME_START() struct timeval td_start,td_end;\
	float elapsed = 0; \
	gettimeofday(&td_start, NULL);
	
#define ELAPSED_TIME_FINISH() 	gettimeofday(&td_end, NULL); \
	elapsed = 1000000.0 * (td_end.tv_sec -td_start.tv_sec); \
	elapsed += (td_end.tv_usec - td_start.tv_usec); \
	elapsed = elapsed / 1000 / 1000; \
	std::cout << "elapsed: " << elapsed << "\n"; \
	exit(0);



template <class T> std::string stringOf(T object) {
	std::ostringstream os;
	os << object;
	return (os.str());
}

/* Replace all instances of from with to in string str in-place */
void replace(std::string &str, const char *from, const char *to, int count = 0);
int isValidEmail(const char *address);
const std::string generateUUID();

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
std::string purpleUsername(const std::string &username);
bool usesJidEscaping(const std::string &name);
bool isValidNode(const std::string &node);

struct replaceBadJidCharacters {
	void operator()(char& c) { if(c == '@') c = '%';}
};

struct replaceJidCharacters {
	void operator()(char& c) { if(c == '%') c = '@'; }
};

bool endsWith(const std::string &str, const std::string &substr);

#if !GLIB_CHECK_VERSION(2,14,0)
GList *g_hash_table_get_keys(GHashTable *table);
#endif

#ifndef WIN32
void process_mem_usage(double& vm_usage, double& resident_set);
#endif

#endif
