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
#include "log.h"

AdhocAdmin::AdhocAdmin(GlooxMessageHandler *m, User *user, const std::string &from, const std::string &id) {
	main = m;
	m_user = user;
	PurpleValue *value;
	Tag *field;
	m_from = std::string(from);
	setRequestType(CALLER_ADHOC);
	
	IQ _response(IQ::Result, from, id);
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	Tag *c = new Tag("command");
	c->addAttribute("xmlns","http://jabber.org/protocol/commands");
	c->addAttribute("sessionid",main->j->getID());
	c->addAttribute("node","transport_settings");
	c->addAttribute("status","executing");

	Tag *actions = new Tag("actions");
	actions->addAttribute("execute","complete");
	actions->addChild(new Tag("complete"));
	c->addChild(actions);

	Tag *xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	xdata->addChild(new Tag("title","Transport settings"));
	xdata->addChild(new Tag("instructions","Change your transport settings here."));

	field = new Tag("field");
	field->addAttribute("type","boolean");
	field->addAttribute("label","Enable transport");
	field->addAttribute("var","enable_transport");
	value = m_user->getSetting("enable_transport");
	if (purple_value_get_boolean(value))
		field->addChild(new Tag("value","1"));
	else
		field->addChild(new Tag("value","0"));
	xdata->addChild(field);

	field = new Tag("field");
	field->addAttribute("type","boolean");
	field->addAttribute("label","Enable network notification");
	field->addAttribute("var","enable_notify_email");
	value = m_user->getSetting("enable_notify_email");
	if (purple_value_get_boolean(value))
		field->addChild(new Tag("value","1"));
	else
		field->addChild(new Tag("value","0"));
	xdata->addChild(field);

	field = new Tag("field");
	field->addAttribute("type","boolean");
	field->addAttribute("label","Enable avatars");
	field->addAttribute("var","enable_avatars");
	value = m_user->getSetting("enable_avatars");
	if (purple_value_get_boolean(value))
		field->addChild(new Tag("value","1"));
	else
		field->addChild(new Tag("value","0"));
	xdata->addChild(field);

	field = new Tag("field");
	field->addAttribute("type","boolean");
	field->addAttribute("label","Enable chatstates");
	field->addAttribute("var","enable_chatstate");
	value = m_user->getSetting("enable_chatstate");
	if (purple_value_get_boolean(value))
		field->addChild(new Tag("value","1"));
	else
		field->addChild(new Tag("value","0"));
	xdata->addChild(field);

	c->addChild(xdata);
	response->addChild(c);
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

		Tag *c = new Tag("command");
		c->addAttribute("xmlns","http://jabber.org/protocol/commands");
		c->addAttribute("sessionid",tag->findAttribute("sessionid"));
		c->addAttribute("node","configuration");
		c->addAttribute("status","canceled");
		response->addChild(c);
		main->j->send(response);

// 		g_timeout_add(0,&removeHandler,this);
		delete stanzaTag;
		return true;
	}
	
	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x) {
		std::string result("");
		for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it) {
			std::string key = (*it)->findAttribute("var");
			if (key.empty()) continue;
			
			PurpleValue * savedValue = m_user->getSetting(key.c_str());
			if (!savedValue) continue;
			
			Tag *v =(*it)->findChild("value");
			if (!v) continue;
			
			PurpleValue *value;
			if (purple_value_get_type(savedValue) == PURPLE_TYPE_BOOLEAN) {
				value = purple_value_new(PURPLE_TYPE_BOOLEAN);
				purple_value_set_boolean(value, atoi(v->cdata().c_str()));
				if (purple_value_get_boolean(savedValue) == purple_value_get_boolean(value)) {
					purple_value_destroy(value);
					continue;
				}
				m_user->updateSetting(key, value);
			}
		}

		IQ _s(IQ::Result, stanza.from().full(), stanza.id());
		_s.setFrom(main->jid());
		Tag *s = _s.tag();

		Tag *c = new Tag("command");
		c->addAttribute("xmlns","http://jabber.org/protocol/commands");
		c->addAttribute("sessionid",tag->findAttribute("sessionid"));
		c->addAttribute("node","configuration");
		c->addAttribute("status","completed");
		s->addChild(c);
		main->j->send(s);
		
// 		g_timeout_add(0,&removeRepeater,this);		
	}

	delete stanzaTag;
	return true;
}

