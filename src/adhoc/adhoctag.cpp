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

#include "adhoctag.h"
#include "gloox/stanza.h"

AdhocTag::AdhocTag(const std::string &id, const std::string &node, const std::string &status) : Tag("command") {
	xdata = NULL;
	addAttribute("xmlns", "http://jabber.org/protocol/commands");
	addAttribute("sessionid", id);
	addAttribute("node", node);
	addAttribute("status", status);
}

void AdhocTag::setAction(const std::string &action) {
	Tag *actions = new Tag("actions");
	actions->addAttribute("execute", action);
	actions->addChild(new Tag(action));
	addChild(actions);
}

void AdhocTag::setTitle(const std::string &title) {
	if (xdata == NULL)
		initXData();
	xdata->addChild(new Tag("title", title));
}

void AdhocTag::setInstructions(const std::string &instructions) {
	if (xdata == NULL)
		initXData();
	xdata->addChild(new Tag("instructions", instructions));
}

void AdhocTag::addListSingle(const std::string &label, const std::string &var, std::list <std::string> &values) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "list-single");
	field->addAttribute("label", label);
	field->addAttribute("var", var);

	Tag *option = new Tag("option");
	for (std::list<std::string>::iterator it = values.begin(); it != values.end(); it++) {
		option->addChild( new Tag("value", *it) );
	}
	field->addChild(option);
	xdata->addChild(field);
}

void AdhocTag::addBoolean(const std::string &label, const std::string &var, bool value) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "boolean");
	field->addAttribute("label", label);
	field->addAttribute("var", var);
	
	if (value)
		field->addChild(new Tag("value", "1"));
	else
		field->addChild(new Tag("value", "0"));
	xdata->addChild(field);
}

void AdhocTag::initXData() {
	xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	addChild(xdata);
}
