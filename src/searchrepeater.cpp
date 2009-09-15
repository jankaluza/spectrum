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

#include "searchrepeater.h"
#include "gloox/stanza.h"
#include "log.h"
#include "protocols/abstractprotocol.h"
#include "dataforms.h"
#include "user.h"
#include "main.h"
#include <iostream>

static gboolean removeRepeater(gpointer data){
	SearchRepeater *repeater = (SearchRepeater*) data;
	purple_request_close(repeater->type(),repeater);
	Log().Get("SearchRepeater") << "repeater closed";
	return FALSE;
}

SearchRepeater::SearchRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, const std::string &value, gboolean multiline, gboolean masked, GCallback ok_cb, GCallback cancel_cb, void * user_data) {
	main = m;
	m_user = user;
	m_ok_cb = ok_cb;
	m_lastTag = NULL;
	m_cancel_cb = cancel_cb;
	m_requestData = user_data;
	AdhocData data = user->adhocData();
	m_from = data.from;

	setType(PURPLE_REQUEST_INPUT);
	setRequestType(CALLER_SEARCH);

	// <iq from="vjud.localhost" type="result" to="test@localhost/hanzz-laptop" id="aad1a" >
	// <query xmlns="jabber:iq:search">
	// <instructions>You need an x:data capable client to search</instructions>
	// <x xmlns="jabber:x:data" type="form" >
	// <title>Search users in vjud.localhost</title>
	// <instructions>Fill in the form to search for any matching Jabber User (Add * to the end of field to match substring)</instructions>
	// <field type="text-single" label="User" var="user" />
	// <field type="text-single" label="Full Name" var="fn" />
	// <field type="text-single" label="Name" var="first" />
	// <field type="text-single" label="Middle Name" var="middle" />
	// <field type="text-single" label="Family Name" var="last" />
	// <field type="text-single" label="Nickname" var="nick" />
	// <field type="text-single" label="Birthday" var="bday" />
	// <field type="text-single" label="Country" var="ctry" />
	// <field type="text-single" label="City" var="locality" />
	// <field type="text-single" label="Email" var="email" />
	// <field type="text-single" label="Organization Name" var="orgname" />
	// <field type="text-single" label="Organization Unit" var="orgunit" />
	// </x>
	// </query>
	// </iq>

	IQ _response(IQ::Result, data.from, data.id);
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:search");
	query->addChild( new Tag("instructions", "<instructions>You need an x:data capable client to search</instructions>") );

	query->addChild( xdataFromRequestInput(title, primaryString, value, multiline) );

	response->addChild(query);
	main->j->send(response);
}

SearchRepeater::SearchRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, PurpleRequestFields *fields, GCallback ok_cb, GCallback cancel_cb, void * user_data) {
	setType(PURPLE_REQUEST_FIELDS);
	main = m;
	m_user = user;
	m_ok_cb = ok_cb;
	m_cancel_cb = cancel_cb;
	m_fields = fields;
	m_requestData = user_data;
	AdhocData data = user->adhocData();
	m_from = data.from;
	setRequestType(CALLER_SEARCH);

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

SearchRepeater::~SearchRepeater() {
	if (m_lastTag)
		delete m_lastTag;
}

void SearchRepeater::sendSearchResults(PurpleNotifySearchResults *results) {
	GList *columniter;
	GList *row, *column;
	int i = 0;
	std::map <int, std::string> columns;
	std::cout << "SEARCH RESULTS\n";

	IQ _response(IQ::Result, m_lastTag->findAttribute("from"), m_lastTag->findAttribute("id"));
	Tag *response = _response.tag();
	response->addAttribute("from",main->jid());

	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:search");

	Tag *xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","result");

	Tag *reported = new Tag("reported");

	Tag *field = new Tag("field");
	field->addAttribute("var","jid");
	field->addAttribute("label","Jabber ID");
	field->addAttribute("type","jid-single");
	reported->addChild(field);

	for (columniter = results->columns; columniter != NULL; columniter = columniter->next) {
		PurpleNotifySearchColumn *column = (PurpleNotifySearchColumn *) columniter->data;
		if (column) {
			std::string c(column->title);
			field = new Tag("field");
			field->addAttribute("var",c);
			field->addAttribute("label",c);
			field->addAttribute("type","text-single");
			reported->addChild(field);
			columns[i] = c;
		}
		i++;
	}

	xdata->addChild(reported);
	std::string IDColumn = main->protocol()->userSearchColumn();

	for (row = results->rows; row != NULL; row = row->next) {
		Tag *item = new Tag("item");
		i = 0;
		for (column = (GList *) row->data; column != NULL; column = column->next) {
			if (column->data) {
				std::string columnText((const char *) column->data);
				Tag *value = new Tag("value", columnText);
				field = new Tag("field");
				field->addAttribute("var", columns[i]);
				field->addChild(value);
				item->addChild(field);
				if (columns[i] == IDColumn) {
					value = new Tag("value", columnText + "@" + main->jid());
					field = new Tag("field");
					field->addAttribute("var", "jid");
					field->addChild(value);
					item->addChild(field);
				}
			}
			i++;
		}
		xdata->addChild(item);
	}

	query->addChild(xdata);
	response->addChild(query);
	main->j->send(response);

	g_timeout_add(0,&removeRepeater,this);
}

bool SearchRepeater::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag) return false;

	Tag * tag = stanzaTag->findChild("query");

	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x) {
		if (m_type == PURPLE_REQUEST_FIELDS) {
			setRequestFields(m_fields, x);
			((PurpleRequestFieldsCb) m_ok_cb) (m_requestData, m_fields);
		}
		else if (m_type == PURPLE_REQUEST_INPUT) {
			std::string result("");
			for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it){
				if ((*it)->hasAttribute("var","result")){
					result = (*it)->findChild("value")->cdata();
					break;
				}
			}

			((PurpleRequestInputCb) m_ok_cb)(m_requestData, result.c_str());
		}
	}

	m_lastTag = stanzaTag;
	return false;
}

