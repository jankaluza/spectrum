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
#include <sstream>
#include <fstream>
#include "gloox/jid.h"
#include <algorithm>
#include "protocols/abstractprotocol.h"
#include "transport.h"
#include <sys/param.h>
#ifdef BSD
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#endif /* BSD */

using namespace gloox;

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

void replace(std::string &str, const char *from, const char *to, int count)
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
		count--;
		if (count == 0)
			return;
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

#ifndef WIN32
#ifdef BSD
void process_mem_usage(double& vm_usage, double& resident_set) {
	kvm_t *kd;
	struct kinfo_proc *ki;
	int pagesize,cnt,size;

	size = sizeof(pagesize);
	sysctlbyname("hw.pagesize",&pagesize,&size,NULL,0);

	kd = kvm_open(getbootfile(),"/dev/null",NULL,O_RDONLY,err);
	ki = kvm_getprocs(kd,KERN_PROC_PID,pid,&cnt);

	vm_usage = (double) ki->ki_size/1024;
	resident_set = (double) (ki->ki_rssize*pagesize)/1024;
	kvm_close(kd);
	free(err);
}
#else /* BSD */
void process_mem_usage(double& vm_usage, double& resident_set) {
	using std::ios_base;
	using std::ifstream;
	using std::string;

	vm_usage     = 0.0;
	resident_set = 0.0;

	// 'file' stat seems to give the most reliable results
	//
	ifstream stat_stream("/proc/self/stat",ios_base::in);
	if (!stat_stream.is_open()) {
		vm_usage = 0;
		resident_set = 0;
		return;
	}

	// dummy vars for leading entries in stat that we don't care about
	//
	string pid, comm, state, ppid, pgrp, session, tty_nr;
	string tpgid, flags, minflt, cminflt, majflt, cmajflt;
	string utime, stime, cutime, cstime, priority, nice;
	string O, itrealvalue, starttime;

	// the two fields we want
	//
	unsigned long vsize;
	long rss;

	stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
				>> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
				>> utime >> stime >> cutime >> cstime >> priority >> nice
				>> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

	long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
	vm_usage     = vsize / 1024.0;
	resident_set = rss * page_size_kb;
}
#endif /* else BSD */
#endif /* WIN32 */

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

std::string purpleUsername(const std::string &username) {
	std::string uname = username;
	std::string unescapedUname = JID::unescapeNode(uname);
	// Check if there was \40 (if JID uses escaping) and if not, replace % by @
	if (unescapedUname == uname) {
// 		std::for_each( uname.begin(), uname.end(), replaceJidCharacters() );
		if (uname.find_last_of("%") != std::string::npos) {
			uname.replace(uname.find_last_of("%"), 1, "@");
		}
	}
	else
		uname = unescapedUname;
	return uname;
}

bool usesJidEscaping(const std::string &name) {
#define CONTAINS(A) (name.find(A) != std::string::npos)
	bool usesEscaping = !(!CONTAINS("\\20") && !CONTAINS("\\22") && !CONTAINS("\\26") && !CONTAINS("\\27")
						&& !CONTAINS("\\2f") && !CONTAINS("\\3a") && !CONTAINS("\\3c") && !CONTAINS("\\3e")
						&& !CONTAINS("\\40") && !CONTAINS("\\5c"));
#undef CONTAINS
	return usesEscaping;
}

bool isValidNode(const std::string &node) {
	bool valid = node.find("\n") == std::string::npos;
	return valid;
}

bool endsWith(const std::string &str, const std::string &substr) {
	size_t i = str.rfind(substr);
	return (i != std::string::npos) && (i == (str.length() - substr.length()));
}

#if !GLIB_CHECK_VERSION(2,14,0)
static void get_key(void *key, void */*value*/, void *data) {
	GList *l = (GList *) data;
	l = g_list_prepend (l, key); 
}

GList *g_hash_table_get_keys(GHashTable *table) {
	GList *l = NULL;
	g_hash_table_foreach(table, get_key, l);
	return l;
}
#endif

void sendError(int code, const std::string &err, const Tag *iqTag) {
	Tag *iq = new Tag("iq");
	iq->addAttribute("type", "error");
	iq->addAttribute("from", Transport::instance()->jid());
	iq->addAttribute("to", iqTag->findAttribute("from"));
	iq->addAttribute("id", iqTag->findAttribute("id"));

	Tag *error = new Tag("error");
	error->addAttribute("code", code);
	error->addAttribute("type", err == "not-allowed" ? "cancel" : "modify");
	Tag *bad = new Tag(err);
	bad->addAttribute("xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas");

	error->addChild(bad);
	iq->addChild(error);

	Transport::instance()->send(iq);
}

void sendError(int code, const std::string &err, const IQ &stanza) {
	Tag *iq = stanza.tag();
	sendError(code, err, iq);
	delete iq;
}
