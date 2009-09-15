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

#include "registerhandler.h"
#include "main.h"
#include <time.h>
#include <gloox/clientbase.h>
#include <gloox/tag.h>
#include <glib.h>
#include "usermanager.h"

#include "sql.h"
#include <sstream>

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
	Log().Get("StatsExtension") << "deleting StatsExtension()";
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

GlooxStatsHandler::GlooxStatsHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
	m_messagesIn = m_messagesOut = 0;
	m_startTime = time(NULL);
	p->j->registerStanzaExtension( new StatsExtension() );
}

GlooxStatsHandler::~GlooxStatsHandler(){
}

bool GlooxStatsHandler::handleIq (const IQ &stanza){
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

	Tag *stanzaTag = stanza.tag();
	Tag *query = stanzaTag->findChild("query");
	if (query==NULL) {
		delete stanzaTag;
		return true;
	}
	std::cout << "*** "<< stanza.from().full() << ": received stats request\n";
	std::list<Tag*> stats = query->children();
	if (stats.empty()){
		std::cout << "* sending stats keys\n";
		IQ _s(IQ::Result, stanza.from(), stanza.id());
		std::string from;
		from.append(p->jid());
		_s.setFrom(from);

		Tag *s = _s.tag();
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
		t->addAttribute("name","legacy-network-users/online");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/registered");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/in");
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/out");
		query->addChild(t);

		s->addChild(query);

		p->j->send( s );

	}
	else{
		std::cout << "* sending stats values\n";
		IQ _s(IQ::Result, stanza.from(), stanza.id());
		std::string from;
		from.append(p->jid());
		_s.setFrom(from);

		Tag *s = _s.tag();
		Tag *query = new Tag("query");
		query->setXmlns("http://jabber.org/protocol/stats");

		Tag *t;
		long registered = p->sql()->getRegisteredUsersCount();
		long registeredUsers = p->sql()->getRegisteredUsersRosterCount();

		std::stringstream out;
		out << registered;

		time_t seconds = time(NULL);

		long users = p->userManager()->onlineUserCount();

		t = new Tag("stat");
		t->addAttribute("name","uptime");
		t->addAttribute("units","seconds");
		t->addAttribute("value",seconds - m_startTime);
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/registered");
		t->addAttribute("units","users");
		t->addAttribute("value",out.str());
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/online");
		t->addAttribute("units","users");
		t->addAttribute("value",p->userManager()->userCount());
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/registered");
		t->addAttribute("units","users");
		t->addAttribute("value",registeredUsers);
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/online");
		t->addAttribute("units","users");
		t->addAttribute("value",users);
		query->addChild(t);


		t = new Tag("stat");
		t->addAttribute("name","messages/in");
		t->addAttribute("units","messages");
		t->addAttribute("value",m_messagesIn);
		query->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/out");
		t->addAttribute("units","messages");
		t->addAttribute("value",m_messagesOut);
		query->addChild(t);

		s->addChild(query);

		p->j->send(s);

	}

	delete stanzaTag;
	return true;
}

void GlooxStatsHandler::handleIqID (const IQ &iq, int context){
}
