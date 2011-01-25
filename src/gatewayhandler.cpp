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
#include "log.h"
#include "spectrum_util.h"
#include "usermanager.h"
#include "transport.h"
#include "user.h"

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

GlooxGatewayHandler::GlooxGatewayHandler() : IqHandler(){
	Transport::instance()->registerStanzaExtension( new GatewayExtension() );
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
		_s.setFrom(Transport::instance()->jid());
		Tag *s = _s.tag();
		Tag *query = new Tag("query");
		query->setXmlns("jabber:iq:gateway");

		const char *language = Transport::instance()->getConfiguration().language.c_str();
		query->addChild(new Tag("desc",tr(language, _("Please enter the legacy ID of your contact."))));
		query->addChild(new Tag("prompt",tr(language, _("Contact ID"))));
		s->addChild(query);

		Transport::instance()->send( s );
		return true;
	}
	else if(stanza.subtype() == IQ::Set){
		Tag *stanzaTag = stanza.tag();
		Log("tag", stanzaTag->xml());
		Tag *query = stanzaTag->findChild("query");
		if (query==NULL)
			return false;
		Tag *prompt = query->findChild("prompt");
		if (prompt==NULL)
			return false;
		std::string uin(prompt->cdata());
		
		User *user = Transport::instance()->userManager()->getUserByJID(stanza.from().bare());
		PurpleAccount *account = user ? user->account() : NULL;

		Transport::instance()->protocol()->prepareUsername(uin, account);
		uin = JID::escapeNode(uin);

		IQ _s(IQ::Result, stanza.from(), stanza.id());
		_s.setFrom(Transport::instance()->jid());
		Tag *s = _s.tag();
		query = new Tag("query");
		query->setXmlns("jabber:iq:gateway");

		query->addChild(new Tag("jid",uin+"@"+Transport::instance()->jid()));
		query->addChild(new Tag("prompt",uin+"@"+Transport::instance()->jid()));

		s->addChild(query);

		Transport::instance()->send( s );
		delete stanzaTag;
		return true;
	}

	return false;
}

void GlooxGatewayHandler::handleIqID (const IQ &iq, int context){
}
