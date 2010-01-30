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

#include "adhocadmin.h"
#include "gloox/stanza.h"
#include "user.h"
#include "main.h"
#include "../log.h"
#include "adhoctag.h"

AdhocAdmin::AdhocAdmin(GlooxMessageHandler *m, User *user, const std::string &from, const std::string &id) {
	main = m;
	m_from = std::string(from);
	m_state = ADHOC_ADMIN_INIT;

	IQ _response(IQ::Result, from, id);
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	AdhocTag *adhocTag = new AdhocTag(main->j->getID(), "transport_admin", "executing");
	adhocTag->setAction("next");
	adhocTag->setTitle("Transport administration");
	adhocTag->setInstructions("Please select the group you want to change.");
	
	std::list <std::string> values;
	values.push_back("Logging");
	adhocTag->addListSingle("Config area", "config_area", values);

	response->addChild(adhocTag);
	main->j->send(response);

}

AdhocAdmin::~AdhocAdmin() {}

bool AdhocAdmin::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	Tag *tag = stanzaTag->findChild( "command" );
	if (tag->hasAttribute("action","cancel")){
		IQ _response(IQ::Result, stanza.from().full(), stanza.id());
		_response.setFrom(main->jid());
		Tag *response = _response.tag();
		response->addChild( new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "canceled") );
		main->j->send(response);

		delete stanzaTag;
		return true;
	}

	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x) {
		if (m_state == ADHOC_ADMIN_INIT) {
			std::string result("");
			for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it) {
				if ((*it)->hasAttribute("var","config_area")){
					result = (*it)->findChild("value")->cdata();
					break;
				}
			}
			
			if (result == "Logging") {
				m_state = ADHOC_ADMIN_LOGGING;

				IQ _response(IQ::Result, stanza.from().full(), stanza.id());
				Tag *response = _response.tag();
				response->addAttribute("from", main->jid());

				AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
				adhocTag->setAction("complete");
				adhocTag->setTitle("Logging settings");
				adhocTag->setInstructions("You can change logging settings here.");
				adhocTag->addBoolean("Log XML", "log_xml", main->configuration().logAreas & LOG_AREA_XML);
				adhocTag->addBoolean("Log Purple messages", "log_purple", main->configuration().logAreas & LOG_AREA_PURPLE);

				response->addChild(adhocTag);
				main->j->send(response);
				delete stanzaTag;
				return false;
			}
		}
		else if (m_state == ADHOC_ADMIN_LOGGING) {
			for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it) {
				std::string key = (*it)->findAttribute("var");
				if (key.empty()) continue;

				Tag *v =(*it)->findChild("value");
				if (!v) continue;
				
				std::string data(v->cdata());
				
				if (key == "log_xml") {
					if (data == "1")
						main->configuration().logAreas |= LOG_AREA_XML;
					else {
						Log("test", main->configuration().logAreas);
						main->configuration().logAreas &= ~LOG_AREA_XML;
						Log("test", main->configuration().logAreas);
					}
				}
				else if (key == "log_purple") {
					if (data == "1") {
						main->configuration().logAreas |= LOG_AREA_PURPLE;
						purple_debug_set_enabled(true);
					}
					else {
						main->configuration().logAreas &= ~(LOG_AREA_PURPLE);
						purple_debug_set_enabled(false);
					}
				}
			}
			IQ _response(IQ::Result, stanza.from().full(), stanza.id());
			_response.setFrom(main->jid());
			Tag *response = _response.tag();
			response->addChild( new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed") );
			main->j->send(response);
		}

// 		g_timeout_add(0,&removeRepeater,this);
	}

	delete stanzaTag;
	return true;
}

