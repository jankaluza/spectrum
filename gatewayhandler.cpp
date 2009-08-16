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

#include "gatewayhandler.h"
#include <glib.h>
#include "main.h"
#include "sql.h"

GatewayExtension::GatewayExtension() : StanzaExtension( ExtGateway )
{
	m_tag = NULL;
}

GatewayExtension::GatewayExtension(const Tag *tag) : StanzaExtension( ExtGateway )
{
	m_tag = tag->clone();
}

GatewayExtension::~GatewayExtension()
{
	Log().Get("GatewayExtension") << "deleting GatewayExtension()";
	delete m_tag;
}

const std::string& GatewayExtension::filterString() const
{
	static const std::string filter = "iq/query[@xmlns='jabber:iq:gateway']";
	return filter;
}

Tag* GatewayExtension::tag() const
{
	return m_tag->clone();
}

GlooxGatewayHandler::GlooxGatewayHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
	p->j->registerStanzaExtension( new GatewayExtension() );
}

GlooxGatewayHandler::~GlooxGatewayHandler(){
}

bool GlooxGatewayHandler::handleIq (const IQ &stanza){

	if(stanza.subtype() == IQ::Get) {
		//   <iq type='result' from='aim.jabber.org' to='stpeter@jabber.org/roundabout' id='gate1'>
		//     <query xmlns='jabber:iq:gateway'>
		//       <desc>
		//         Please enter the AOL Screen Name of the
		//         person you would like to contact.
		//       </desc>
		//       <prompt>Contact ID</prompt>
		//     </query>
		//   </iq>
		IQ _s(IQ::Result, stanza.from(), stanza.id());
		_s.setFrom(p->jid());
		Tag *s = _s.tag();
		Tag *query = new Tag("query");
		query->setXmlns("jabber:iq:gateway");
		
		query->addChild(new Tag("desc","Please enter the ICQ Number of the person you would like to contact."));
		query->addChild(new Tag("prompt","Contact ID"));
		s->addChild(query);
		
		p->j->send( s );
		return true;
	}
	else if(stanza.subtype() == IQ::Set){
		Tag *stanzaTag = stanza.tag();
		Log().Get("tag") << stanzaTag->xml();
		Tag *query = stanzaTag->findChild("query");
		if (query==NULL)
			return false;
		Tag *prompt = query->findChild("prompt");
		if (prompt==NULL)
			return false;
		std::string uin(prompt->cdata());
		
		// TODO: rewrite to support more protocols
		replace(uin,"-","");
		replace(uin," ","");
		
		IQ _s(IQ::Result, stanza.from(), stanza.id());
		_s.setFrom(p->jid());
		Tag *s = _s.tag();
		query = new Tag("query");
		query->setXmlns("jabber:iq:gateway");
		
		query->addChild(new Tag("jid",uin+"@"+p->jid()));
		query->addChild(new Tag("prompt",uin+"@"+p->jid()));
		
		s->addChild(query);
		
		p->j->send( s );
		delete stanzaTag;
		return true;
	}

	return false;
}

void GlooxGatewayHandler::handleIqID (const IQ &iq, int context){
}
