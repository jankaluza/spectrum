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

GlooxStatsHandler::GlooxStatsHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
	m_messagesIn = m_messagesOut = 0;
	m_startTime = time(NULL);
}

GlooxStatsHandler::~GlooxStatsHandler(){
}

bool GlooxStatsHandler::handleIq (Stanza *stanza){
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

	Tag *query = stanza->findChild("query");
	if (query==NULL)
		return true;
	std::cout << "*** "<< stanza->from().full() << ": received stats request\n";
	std::list<Tag*> stats = query->children();
	if (stats.empty()){
		std::cout << "* sending stats keys\n";
		Stanza *s = Stanza::createIqStanza(stanza->from(), stanza->id(), StanzaIqResult, "http://jabber.org/protocol/stats");
		std::string from;
		from.append(p->jid());
		s->addAttribute("from",from);
		Tag *t;

		t = new Tag("stat");
		t->addAttribute("name","uptime");
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/registered");
		s->findChild("query")->addChild(t);
		
		t = new Tag("stat");
		t->addAttribute("name","users/online");
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/online");
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/registered");
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/in");
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/out");
		s->findChild("query")->addChild(t);

		p->j->send( s );

	}
	else{
		std::cout << "* sending stats values\n";
		Stanza *s = Stanza::createIqStanza(stanza->from(), stanza->id(), StanzaIqResult, "http://jabber.org/protocol/stats");
		std::string from;
		from.append(p->jid());
		s->addAttribute("from",from);
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
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/registered");
		t->addAttribute("units","users");
		t->addAttribute("value",out.str());
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","users/online");
		t->addAttribute("units","users");
		t->addAttribute("value",p->userManager()->userCount());
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/registered");
		t->addAttribute("units","users");
		t->addAttribute("value",registeredUsers);
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","legacy-network-users/online");
		t->addAttribute("units","users");
		t->addAttribute("value",users);
		s->findChild("query")->addChild(t);
		

		t = new Tag("stat");
		t->addAttribute("name","messages/in");
		t->addAttribute("units","messages");
		t->addAttribute("value",m_messagesIn);
		s->findChild("query")->addChild(t);

		t = new Tag("stat");
		t->addAttribute("name","messages/out");
		t->addAttribute("units","messages");
		t->addAttribute("value",m_messagesOut);
		s->findChild("query")->addChild(t);


		p->j->send(s);

	}

	return true;
}

bool GlooxStatsHandler::handleIqID (Stanza *stanza, int context){
	return false;
}
