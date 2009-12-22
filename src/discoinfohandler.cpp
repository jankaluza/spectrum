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

#include "discoinfohandler.h"
#include "protocols/abstractprotocol.h"
#include "transport.h"
#include "log.h"

GlooxDiscoInfoHandler::GlooxDiscoInfoHandler() : IqHandler() {
}

GlooxDiscoInfoHandler::~GlooxDiscoInfoHandler(){
}


bool GlooxDiscoInfoHandler::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag) return true;
	return handleIq(stanzaTag);
}

bool GlooxDiscoInfoHandler::handleIq(Tag *stanzaTag) {
	if (stanzaTag->findAttribute("type") != "get")
		return true;

	if (JID(stanzaTag->findAttribute("to")).username() != "") {
		Tag *query = stanzaTag->findChildWithAttrib("xmlns", "http://jabber.org/protocol/disco#info");
		if (query) {
			IQ _s(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
			_s.setFrom(stanzaTag->findAttribute("to"));
			Tag *s = _s.tag();

			Tag *query2 = new Tag("query");
			query2->setXmlns("http://jabber.org/protocol/disco#info");
			query2->addAttribute("node", query->findAttribute("node"));

			Tag *t;
			t = new Tag("identity");
			t->addAttribute("category", "gateway");
			t->addAttribute("name", "Spectrum");
			t->addAttribute("type",Transport::instance()->protocol()->gatewayIdentity());
			query2->addChild(t);

			std::list <std::string> features = Transport::instance()->protocol()->buddyFeatures();
			for (std::list <std::string>::iterator it = features.begin(); it != features.end(); it++) {
				t = new Tag("feature");
				t->addAttribute("var", *it);
				query2->addChild(t);
			}

			s->addChild(query2);
			Transport::instance()->send(s);
		}
	}
	else {
		Tag *query = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
		std::string node = query->findAttribute("node");
		// All adhoc commands nodes starts with "transport_" prefix.
		if (node.find("transport_") == 0) {
			IQ _s(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
			_s.setFrom(stanzaTag->findAttribute("to"));
			Tag *s = _s.tag();

			Tag *query2 = new Tag("query");
			query2->setXmlns("http://jabber.org/protocol/disco#info");
			query2->addAttribute("node", node);

			Tag *t;
			t = new Tag("identity");
			t->addAttribute("category","gateway");
			t->addAttribute("type", Transport::instance()->protocol()->gatewayIdentity());
			query2->addChild(t);

			t = new Tag("feature");
			t->addAttribute("var", "http://jabber.org/protocol/commands");
			query2->addChild(t);

			t = new Tag("feature");
			t->addAttribute("var", "jabber:x:data");
			query2->addChild(t);

			s->addChild(query2);
			Transport::instance()->send( s );
		}
	}

	delete stanzaTag;
	return true;
}

void GlooxDiscoInfoHandler::handleIqID (const IQ &stanza, int context){
// 	p->j->disco()->handleIqID(stanza,context);
}

