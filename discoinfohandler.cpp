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

GlooxDiscoInfoHandler::GlooxDiscoInfoHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
}

GlooxDiscoInfoHandler::~GlooxDiscoInfoHandler(){
}


bool GlooxDiscoInfoHandler::handleIq (const IQ &stanza){

/*	User *user = p->getUserByJID(stanza.from().bare());
	if (user==NULL)
		return true;

	if(stanza.subtype() == IQ::Get) {
		std::list<std::string> temp;
		temp.push_back((std::string)stanza.id());
		temp.push_back((std::string)stanza.from().full());
		vcardRequests[(std::string)stanza.to().username()]=temp;
		
		serv_get_info(purple_account_get_connection(user->account), stanza.to().username().c_str());
	}*/
/*
<iq from='romeo@montague.lit/orchard' 
    id='disco1'
    to='juliet@capulet.lit/chamber' 
    type='result'>
  <query xmlns='http://jabber.org/protocol/disco#info'
         node='http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0='>
    <identity category='client' name='Exodus 0.9.1' type='pc'/>
    <feature var='http://jabber.org/protocol/caps'/>
    <feature var='http://jabber.org/protocol/disco#info'/>
    <feature var='http://jabber.org/protocol/disco#items'/>
    <feature var='http://jabber.org/protocol/muc'/>
  </query>
</iq>*/

	std::cout << "DISCO DISCO DISCO INFO\n";
	if(stanza.subtype() == IQ::Get && stanza.to().username()!="") {
		Tag *stanzaTag = stanza.tag();
		if (!stanzaTag) return true;
		Tag *query = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
		if (query!=NULL){
			IQ _s(IQ::Result, stanza.from(), stanza.id());
			_s.setFrom(stanza.to().full());
			Tag *s = _s.tag();
			Tag *query2 = new Tag("query");
			query2->setXmlns("http://jabber.org/protocol/disco#info");
			query2->addAttribute("node",query->findAttribute("node"));
			
			Tag *t;
			t = new Tag("identity");
			t->addAttribute("category","gateway");
			t->addAttribute("name","High Flyer Transport");
			t->addAttribute("type",p->protocol()->gatewayIdentity());
			query2->addChild(t);

			std::list<std::string> features = p->protocol()->transportFeatures();
			for(std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
				t = new Tag("feature");
				t->addAttribute("var",*it);
				query2->addChild(t);
			}

			s->addChild(query2);
			p->j->send( s );

			delete stanzaTag;
			return true;
		}
	}
	else if (stanza.subtype() == IQ::Get) {
		Tag *tag = stanza.tag();
		Tag *query = tag->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
		std::string node = query->findAttribute("node");
		if (node.find("transport_") == 0) {
// 			<iq type='result'
// 				to='requester@domain'
// 				from='responder@domain'>
// 			<query xmlns='http://jabber.org/protocol/disco#info'
// 					node='config'>
// 				<identity name='Configure Service'
// 						category='automation'
// 						type='command-node'/>
// 				<feature var='http://jabber.org/protocol/commands'/>
// 				<feature var='jabber:x:data'/>
// 			</query>
// 			</iq>
			IQ _s(IQ::Result, stanza.from(), stanza.id());
			_s.setFrom(stanza.to().full());
			Tag *s = _s.tag();
			Tag *query2 = new Tag("query");
			query2->setXmlns("http://jabber.org/protocol/disco#info");
			query2->addAttribute("node",node);
			
			Tag *t;
			t = new Tag("identity");
			t->addAttribute("category","gateway");
// 			t->addAttribute("name","High Flyer Transport");
			t->addAttribute("type",p->protocol()->gatewayIdentity());
			query2->addChild(t);

			t = new Tag("feature");
			t->addAttribute("var","http://jabber.org/protocol/commands");
			query2->addChild(t);

			t = new Tag("feature");
			t->addAttribute("var","jabber:x:data");
			query2->addChild(t);

			s->addChild(query2);
			p->j->send( s );
		}
		delete tag;
	}
	
	return true;
}

void GlooxDiscoInfoHandler::handleIqID (const IQ &stanza, int context){
	p->j->disco()->handleIqID(stanza,context);
}

