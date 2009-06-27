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
 
#include "adhocrepeater.h"
#include "gloox/stanza.h"
#include "log.h"
#include "dataforms.h"
 
static gboolean removeRepeater(gpointer data){
	AdhocRepeater *repeater = (AdhocRepeater*) data;
	purple_request_close(repeater->type(),repeater);
	Log().Get("AdhocRepeater") << "repeater closed";
	return FALSE;
}
 
AdhocRepeater::AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, const std::string &value, gboolean multiline, gboolean masked, GCallback ok_cb, GCallback cancel_cb, void * user_data) {
	main = m;
	m_user = user;
	m_ok_cb = ok_cb;
	m_cancel_cb = cancel_cb;
	m_requestData = user_data;
	AdhocData data = user->adhocData();
	m_from = data.from;
	
	setType(PURPLE_REQUEST_INPUT);
	setRequestType(CALLER_ADHOC);
	
	IQ _response(IQ::Result, data.from, data.id);
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	Tag *c = new Tag("command");
	c->addAttribute("xmlns","http://jabber.org/protocol/commands");
	c->addAttribute("sessionid",main->j->getID());
	c->addAttribute("node",data.node);
	c->addAttribute("status","executing");

	Tag *actions = new Tag("actions");
	actions->addAttribute("execute","complete");
	actions->addChild(new Tag("complete"));
	c->addChild(actions);

	c->addChild( xdataFromRequestInput(title, primaryString, value, multiline) );
	
	m_defaultString = value;
	
	response->addChild(c);
	main->j->send(response);

}

AdhocRepeater::AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, int default_action, void * user_data, size_t action_count, va_list acts) {
	setType(PURPLE_REQUEST_ACTION);
	main = m;
	m_user = user;
	m_requestData = user_data;
	AdhocData data = user->adhocData();
	m_from = data.from;
	setRequestType(CALLER_ADHOC);
//       <field type='list-single'
//              label='Maximum number of subscribers'
//              var='maxsubs'>
//         <value>20</value>
//         <option label='10'><value>10</value></option>
//         <option label='20'><value>20</value></option>
//         <option label='30'><value>30</value></option>
//         <option label='50'><value>50</value></option>
//         <option label='100'><value>100</value></option>
//         <option label='None'><value>none</value></option>
//       </field>

	IQ _response(IQ::Result, data.from, data.id);
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	Tag *c = new Tag("command");
	c->addAttribute("xmlns","http://jabber.org/protocol/commands");
	c->addAttribute("sessionid",main->j->getID());
	c->addAttribute("node",data.node);
	c->addAttribute("status","executing");

	Tag *actions = new Tag("actions");
	actions->addAttribute("execute","complete");
	actions->addChild(new Tag("complete"));
	c->addChild(actions);

	for (unsigned int i = 0; i < action_count; i++)
		m_actions[i] = va_arg(acts, GCallback);

	c->addChild( xdataFromRequestAction(title, primaryString, action_count, acts) );
	response->addChild(c);
	main->j->send(response);

}

AdhocRepeater::AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, PurpleRequestFields *fields, GCallback ok_cb, GCallback cancel_cb, void * user_data) {
	setType(PURPLE_REQUEST_FIELDS);
	main = m;
	m_user = user;
	m_ok_cb = ok_cb;
	m_cancel_cb = cancel_cb;
	m_fields = fields;
	m_requestData = user_data;
	AdhocData data = user->adhocData();
	m_from = data.from;
	setRequestType(CALLER_ADHOC);
	
	IQ _response(IQ::Result, data.from, data.id);
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	Tag *c = new Tag("command");
	c->addAttribute("xmlns","http://jabber.org/protocol/commands");
	c->addAttribute("sessionid",main->j->getID());
	c->addAttribute("node",data.node);
	c->addAttribute("status","executing");

	Tag *actions = new Tag("actions");
	actions->addAttribute("execute","complete");
	actions->addChild(new Tag("complete"));
	c->addChild(actions);
	
	c->addChild( xdataFromRequestFields(title, primaryString, fields) );
	response->addChild(c);
	main->j->send(response);
}

AdhocRepeater::~AdhocRepeater() {}

bool AdhocRepeater::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag) return false;
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

		if (m_type == PURPLE_REQUEST_FIELDS) {
			((PurpleRequestFieldsCb) m_cancel_cb) (m_requestData, m_fields);
		}
		else if (m_type == PURPLE_REQUEST_INPUT) {
			((PurpleRequestInputCb) m_cancel_cb)(m_requestData, m_defaultString.c_str());
		}

		g_timeout_add(0,&removeRepeater,this);
		delete stanzaTag;
		return false;
	}
	
	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x) {
		if (m_type == PURPLE_REQUEST_FIELDS) {
			for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it){
				std::string key = (*it)->findAttribute("var");
				if (key.empty()) continue;
				
				PurpleRequestField * fld = purple_request_fields_get_field(m_fields, key.c_str());
				if (!fld) continue;
				PurpleRequestFieldType type;
				type = purple_request_field_get_type(fld);
				
				Tag *v =(*it)->findChild("value");
				if (!v) continue;
				
				std::string val = v->cdata();
				if (val.empty() && type != PURPLE_REQUEST_FIELD_STRING)
					continue;
				
				if (type == PURPLE_REQUEST_FIELD_STRING) {
					purple_request_field_string_set_value(fld, (val.empty() ? NULL : val.c_str()));
				}
				else if (type == PURPLE_REQUEST_FIELD_INTEGER) {
					purple_request_field_int_set_value(fld, atoi(val.c_str()));
				}
				else if (type == PURPLE_REQUEST_FIELD_BOOLEAN) {
					purple_request_field_bool_set_value(fld, atoi(val.c_str()));
				}
				else if (type == PURPLE_REQUEST_FIELD_CHOICE) {
					purple_request_field_choice_set_value(fld, atoi(val.c_str()));
				}
				else if (type == PURPLE_REQUEST_FIELD_LIST) {
					for(std::list<Tag*>::const_iterator it2 = (*it)->children().begin(); it2 != (*it)->children().end(); ++it2) {
						val = (*it2)->cdata();
						if (!val.empty())
							purple_request_field_list_add_selected(fld, val.c_str());
					}
				}
			}
			((PurpleRequestFieldsCb) m_ok_cb) (m_requestData, m_fields);
		}
		else {
			std::string result("");
			for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it){
				if ((*it)->hasAttribute("var","result")){
					result = (*it)->findChild("value")->cdata();
					break;
				}
			}

			if (m_type == PURPLE_REQUEST_INPUT) {
				((PurpleRequestInputCb) m_ok_cb)(m_requestData, result.c_str());
			}
			else if (m_type == PURPLE_REQUEST_ACTION) {
				std::istringstream i(result);
				int index;
				i >> index;
				if (m_actions.find(index) != m_actions.end()) {
					PurpleRequestActionCb callback = (PurpleRequestActionCb) m_actions[index];
					if (callback)
						(callback)(m_requestData,index);
				}
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
		
		g_timeout_add(0,&removeRepeater,this);
		
	}

	delete stanzaTag;
	return false;
}

