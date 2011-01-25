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

#include "statshandler.h"
#include "main.h"
#include <time.h>
#include <gloox/clientbase.h>
#include <gloox/tag.h>
#include <glib.h>
#include "usermanager.h"
#include "log.h"
#include "spectrum_util.h"
#include "transport.h"

#include "sql.h"
#include <sstream>
#include <fstream>

StatsExtension::StatsExtension() : StanzaExtension( ExtStats )
{
	m_tag = NULL;
}

StatsExtension::StatsExtension(const Tag *tag) : StanzaExtension( ExtStats )
{
	m_tag = tag->clone();
}

StatsExtension::~StatsExtension()
{
	Log("StatsExtension", "deleting StatsExtension()");
	delete m_tag;
}

const std::string& StatsExtension::filterString() const
{
	static const std::string filter = "iq/query[@xmlns='http://jabber.org/protocol/stats']";
	return filter;
}

Tag* StatsExtension::tag() const
{
	return m_tag->clone();
}

GlooxStatsHandler::GlooxStatsHandler() : IqHandler(){
	m_messagesIn = m_messagesOut = 0;
	m_startTime = time(NULL);
	Transport::instance()->registerStanzaExtension( new StatsExtension() );
}

GlooxStatsHandler::~GlooxStatsHandler(){
}

bool GlooxStatsHandler::handleCondition(Tag *stanzaTag) {
	std::cout << "conditiong" << stanzaTag->findChild("query","xmlns","http://jabber.org/protocol/stats") << "\n";
	return stanzaTag->findChild("query","xmlns","http://jabber.org/protocol/stats") != NULL;
}

Tag* GlooxStatsHandler::handleTag (Tag *stanzaTag){
// 	      recv:     <iq type='result' from='component'>
//                   <query xmlns='http://jabber.org/protocol/stats'>
// 	            <stat name='time/uptime'/>
// 	            <stat name='users/online'/>
// 	                  .
// 	                  .
// 	                  .
// 	            <stat name='bandwidth/packets-in'/>
// 	            <stat name='bandwidth/packets-out'/>
// 	          </query>
//                 </iq>
//
	Tag *s;
	std::cout << stanzaTag->xml();
	Tag *query = stanzaTag->findChild("query");
	if (query==NULL) {
		return NULL;
	}
	std::list<Tag*> stats = query->children();
	if (stats.empty()){
		std::cout << "* sending stats keys\n";
		IQ _s(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
		std::string from;
		from.append(Transport::instance()->jid());
		_s.setFrom(from);

		s = _s.tag();
		Tag *query = new Tag("query");
		query->setXmlns("http://jabber.org/protocol/stats");
		Tag *t;

		t = new Tag("stat");
		t->addAttribute("name","uptime");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/registered");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/online");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","contacts/online");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","contacts/total");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/in");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/out");
		query->addChild(t);
		
#ifndef WIN32
		t = new Tag("stat");
		t->addAttribute("name","memory-usage");
		query->addChild(t);
#endif

		s->addChild(query);

	}
	else{
		std::cout << "* sending stats values\n";
		IQ _s(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
		std::string from;
		from.append(Transport::instance()->jid());
		_s.setFrom(from);

		s = _s.tag();
		Tag *query = new Tag("query");
		query->setXmlns("http://jabber.org/protocol/stats");

		Tag *t;
		long registered = Transport::instance()->sql()->getRegisteredUsersCount();
		long registeredUsers = Transport::instance()->sql()->getRegisteredUsersRosterCount();

		std::stringstream out;
		out << registered;

		time_t seconds = time(NULL);

		long users = Transport::instance()->userManager()->onlineBuddiesCount();

		for (std::list<Tag*>::iterator i = stats.begin(); i != stats.end(); i++) {
			std::string name = (*i)->findAttribute("name");
			if (name == "uptime") {
				t = new Tag("stat");
				t->addAttribute("name","uptime");
				t->addAttribute("units","seconds");
				t->addAttribute("value",seconds - m_startTime);
				query->addChild(t);
			} else if (name == "users/registered") {
				t = new Tag("stat");
				t->addAttribute("name","users/registered");
				t->addAttribute("units","users");
				t->addAttribute("value",out.str());
				query->addChild(t);
			} else if (name == "users/online") {
				t = new Tag("stat");
				t->addAttribute("name","users/online");
				t->addAttribute("units","users");
				t->addAttribute("value",Transport::instance()->userManager()->userCount());
				query->addChild(t);
			} else if (name == "contacts/total") {
				t = new Tag("stat");
				t->addAttribute("name","contacts/total");
				t->addAttribute("units","contacts");
				t->addAttribute("value",registeredUsers);
				query->addChild(t);
			} else if (name == "contacts/online") {
				t = new Tag("stat");
				t->addAttribute("name","contacts/online");
				t->addAttribute("units","contacts");
				t->addAttribute("value",users);
				query->addChild(t);
			} else if (name == "messages/in") {
				t = new Tag("stat");
				t->addAttribute("name","messages/in");
				t->addAttribute("units","messages");
				t->addAttribute("value",m_messagesIn);
				query->addChild(t);
			} else if (name == "messages/out") {
				t = new Tag("stat");
				t->addAttribute("name","messages/out");
				t->addAttribute("units","messages");
				t->addAttribute("value",m_messagesOut);
				query->addChild(t);
			}
#ifndef WIN32
			else if (name == "memory-usage") {
				double vm, rss;
				std::stringstream rss_stream;
				process_mem_usage(vm, rss);
				rss_stream << rss;

				t = new Tag("stat");
				t->addAttribute("name","memory-usage");
				t->addAttribute("units","KB");
				t->addAttribute("value", rss_stream.str());
				query->addChild(t);
			}
#endif
			else {
				t = new Tag("stat");
				t->addAttribute("name", name);
				Tag *error = new Tag("error", "Not Found");
				error->addAttribute("code", "503");
				t->addChild(error);
				query->addChild(t);
			}
		}

		s->addChild(query);
	}
	
	return s;
}

bool GlooxStatsHandler::handleIq (const IQ &stanza){
	std::cout << "*** "<< stanza.from().full() << ": received stats request\n";
	if ((CONFIG().transportFeatures & TRANSPORT_FEATURE_STATISTICS) == 0) {
		sendError(405, "not-allowed", stanza);
		return true;
	}
	Tag *stanzaTag = stanza.tag();
	Tag *query = handleTag(stanzaTag);
	delete stanzaTag;
	if (query)
		Transport::instance()->send(query);

	return true;
}

void GlooxStatsHandler::handleIqID (const IQ &iq, int context){
}
