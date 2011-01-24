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
#include "transport.h"

AdhocTag::AdhocTag(const std::string &id, const std::string &node, const std::string &status) : Tag("command") {
	xdata = NULL;
	addAttribute("xmlns", "http://jabber.org/protocol/commands");
	addAttribute("sessionid", id);
	addAttribute("node", node);
	addAttribute("status", status);
}

AdhocTag::AdhocTag(const IQ &stanza) : Tag("command") {
	m_from = stanza.from().full();
	m_id = stanza.id();
	xdata = NULL;
	Tag *iq = stanza.tag();
	Tag *command = iq->findChild("command");
	if (command) {
		const Tag::AttributeList & attributes = command->attributes();
		for (Tag::AttributeList::const_iterator it = attributes.begin(); it != attributes.end(); it++) {
			addAttribute(std::string((*it)->name()), std::string((*it)->value()));
		}
		Tag *x = command->findChildWithAttrib("xmlns","jabber:x:data");
		if (x) {
			xdata = x->clone();
			addChild(xdata);
		}
	}
	delete iq;
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

	for (std::list<std::string>::iterator it = values.begin(); it != values.end(); it++) {
		Tag *option = new Tag("option");
		option->addChild( new Tag("value", *it) );
		field->addChild(option);
	}
	xdata->addChild(field);
}

void AdhocTag::addListSingle(const std::string &label, const std::string &var, std::map <std::string, std::string> &values) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "list-single");
	field->addAttribute("label", label);
	field->addAttribute("var", var);

	for (std::map<std::string, std::string>::iterator it = values.begin(); it != values.end(); it++) {
		Tag *option = new Tag("option");
		option->addAttribute("label", (*it).first);
		option->addChild( new Tag("value", (*it).second) );
		field->addChild(option);
	}
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

void AdhocTag::addTextSingle(const std::string &label, const std::string &var, const std::string &value) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "text-single");
	field->addAttribute("label", label);
	field->addAttribute("var", var);
	
	field->addChild(new Tag("value", value));
	xdata->addChild(field);
}

void AdhocTag::addTextMulti(const std::string &label, const std::string &var, const std::string &value) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "text-multi");
	field->addAttribute("label", label);
	field->addAttribute("var", var);
	
	field->addChild(new Tag("value", value));
	xdata->addChild(field);
}

void AdhocTag::addTextPrivate(const std::string &label, const std::string &var, const std::string &value) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "text-private");
	field->addAttribute("label", label);
	field->addAttribute("var", var);
	
	field->addChild(new Tag("value", value));
	xdata->addChild(field);
}

void AdhocTag::addFixedText(const std::string &text) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "fixed");
	
	field->addChild(new Tag("value", text));
	xdata->addChild(field);
}

void AdhocTag::addHidden(const std::string &var, const std::string &value) {
	if (xdata == NULL)
		initXData();
	Tag *field = new Tag("field");
	field->addAttribute("type", "hidden");
	field->addAttribute("var", var);
	
	field->addChild(new Tag("value", value));
	xdata->addChild(field);
}

const std::string AdhocTag::getValue(const std::string &var) {
	if (xdata == NULL)
		return "";
	Tag *v = xdata->findChildWithAttrib("var", var);
	if (!v)
		return "";
	Tag *value =v->findChild("value");
	if (!value)
		return "";
	return value->cdata();
}

Tag * AdhocTag::generateResponse(const std::string &action) {
	IQ _response(IQ::Result, m_from, m_id);
	_response.setFrom(Transport::instance()->jid());
	Tag *response = _response.tag();

	if (action != "") {
		response->addChild( new AdhocTag(findAttribute("sessionid"), findAttribute("node"), action) );
		return response;
	}

	if (hasAttribute("action", "cancel"))
		response->addChild( new AdhocTag(findAttribute("sessionid"), findAttribute("node"), "canceled") );
	else
		response->addChild( new AdhocTag(findAttribute("sessionid"), findAttribute("node"), "completed") );
	
	return response;
}

bool AdhocTag::isCanceled() {
	return hasAttribute("action", "cancel");
}

bool AdhocTag::isFinished() {
	return findAttribute("status") != "executing";
}

void AdhocTag::initXData() {
	xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	addChild(xdata);
}

void AdhocTag::addNote(const std::string &type, const std::string &message) {
	Tag *note = new Tag("note", message);
	note->addAttribute("type", type);
	addChild(note);
}
