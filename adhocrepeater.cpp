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
 
AdhocRepeater::AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, const std::string &value, gboolean multiline, gboolean masked, GCallback ok_cb, GCallback cancel_cb, void * user_data) {
	main = m;
	m_user = user;
	m_ok_cb = ok_cb;
	m_cancel_cb = cancel_cb;
	m_requestData = user_data;
	AdhocData data = user->adhocData();
	
	Stanza *response = Stanza::createIqStanza(data.from,data.id,StanzaIqResult,"",0);
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

	Tag *xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	xdata->addChild(new Tag("title",title));
	xdata->addChild(new Tag("instructions",primaryString));

	Tag *field = new Tag("field");
	if (multiline)
		field->addAttribute("type","text-multi");
	else
		field->addAttribute("type","text-single");
	field->addAttribute("label","Field:");
	field->addAttribute("var","result");
	field->addChild(new Tag("value",value));
	xdata->addChild(field);

	c->addChild(xdata);
	response->addChild(c);
	main->j->send(response);

}

AdhocRepeater::~AdhocRepeater() {}

bool AdhocRepeater::handleIq(Stanza *stanza) {
	return true;
}

