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
#include "gloox/dataform.h"
#include "abstractuser.h"
#include "transport.h"
#include "../log.h"
#include "adhoctag.h"
#include "main.h"
#include "Poco/Format.h"

AdhocAdmin::AdhocAdmin(AbstractUser *user, const std::string &from, const std::string &id) {
	setRequestType(CALLER_ADHOC);
	m_from = std::string(from);
	m_state = ADHOC_ADMIN_INIT;
	
	if (user)
		m_language = std::string(user->getLang());
	else
		m_language = Transport::instance()->getConfiguration().language;

	IQ _response(IQ::Result, from, id);
	Tag *response = _response.tag();
	response->addAttribute("from", Transport::instance()->jid());

	AdhocTag *adhocTag = new AdhocTag(Transport::instance()->getId(), "transport_admin", "executing");
	adhocTag->setAction("next");
	adhocTag->setTitle(tr(m_language.c_str(), _("Transport administration")));
	adhocTag->setInstructions(tr(m_language.c_str(), _("Please select a configuration area.")));
	
	std::list <std::string> values;
	values.push_back(tr(m_language.c_str(), _("User")));
	adhocTag->addListSingle("Config area", "config_area", values);

	response->addChild(adhocTag);
	Transport::instance()->send(response);
}

AdhocAdmin::AdhocAdmin() {
	setRequestType(CALLER_ADHOC);
	m_state = ADHOC_ADMIN_INIT;
	
	m_language = Transport::instance()->getConfiguration().language;
}

AdhocAdmin::~AdhocAdmin() {}

bool AdhocAdmin::handleCondition(Tag *stanzaTag) {
	Tag *tag = stanzaTag->findChild("command");
	return (tag && tag->findAttribute("node") == "transport_admin");
}

Tag *AdhocAdmin::handleTag(Tag *stanzaTag) {
	Tag *tag = stanzaTag->findChild("command");
	if (tag->hasAttribute("action", "cancel")) {
		IQ _response(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
		_response.setFrom(Transport::instance()->jid());
		Tag *response = _response.tag();
		response->addChild( new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "canceled") );
		return response;
	}

	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x == NULL)
		return NULL;

	DataForm form(x);
	if (form.type() == TypeInvalid)
		return NULL;

	if (form.hasField("adhoc_state")) {
		const std::string &data = form.field("adhoc_state")->value();
		if (data == "ADHOC_ADMIN_INIT")
			m_state = ADHOC_ADMIN_INIT;
		else if (data == "ADHOC_ADMIN_USER2")
			m_state = ADHOC_ADMIN_USER2;
	}

	if (m_state == ADHOC_ADMIN_INIT) {
		if (!form.hasField("config_area"))
			return NULL;

		const std::string &result = form.field("config_area")->value();
		if (result == tr(m_language.c_str(), _("User"))) {
			m_state = ADHOC_ADMIN_USER;

			IQ _response(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
			Tag *response = _response.tag();
			response->addAttribute("from", Transport::instance()->jid());

			AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
			adhocTag->setAction("next");
			adhocTag->setTitle(tr(m_language.c_str(), _("User information")));
			adhocTag->setInstructions(tr(m_language.c_str(), _("Add bare JID of user you want to configure.")));
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Bare JID")), "user_jid");

			response->addChild(adhocTag);
			return response;
		}
	}
	else if (m_state == ADHOC_ADMIN_USER) {
		m_state = ADHOC_ADMIN_USER2;
		if (!form.hasField("user_jid"))
			return NULL;
		const std::string &user_jid = form.field("user_jid")->value();
			
		IQ _response(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
		Tag *response = _response.tag();
		response->addAttribute("from", Transport::instance()->jid());

		AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
		adhocTag->setAction("complete");
		adhocTag->setTitle(tr(m_language.c_str(), _("User information")));
		adhocTag->setInstructions(tr(m_language.c_str(), _("User information")));
		adhocTag->addHidden("user_jid", user_jid);

		UserRow u = Transport::instance()->sql()->getUserByJid(user_jid);
		if (u.id == -1) {
			adhocTag->addFixedText(tr(m_language.c_str(), _("This user is not registered.")));
		}
		else {
			adhocTag->addFixedText(tr(m_language.c_str(), Poco::format(_("JID: %s"), u.jid)));
			adhocTag->addFixedText(tr(m_language.c_str(), Poco::format(_("Legacy network name: %s"), u.uin)));
			adhocTag->addFixedText(tr(m_language.c_str(), Poco::format(_("Language: %s"), u.language)));
			adhocTag->addFixedText(tr(m_language.c_str(), Poco::format(_("Encoding: %s"), u.encoding)));
			adhocTag->addBoolean(tr(m_language.c_str(), _("VIP")), "user_vip", u.vip);
		}
		
		response->addChild(adhocTag);
		return response;
	}
	else if (m_state == ADHOC_ADMIN_USER2) {
		if (!form.hasField("user_jid") || !form.hasField("user_vip"))
			return NULL;
		const std::string &user_jid = form.field("user_jid")->value();
		const std::string &user_vip = form.field("user_vip")->value();

		UserRow u = Transport::instance()->sql()->getUserByJid(user_jid);
		if (u.id != -1) {
			u.vip = user_vip == "1";
			Transport::instance()->sql()->updateUser(u);
		}

		IQ _response(IQ::Result, stanzaTag->findAttribute("from"), stanzaTag->findAttribute("id"));
		_response.setFrom(Transport::instance()->jid());
		Tag *response = _response.tag();
		response->addChild( new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed") );
		return response;
	}
	return NULL;
}

bool AdhocAdmin::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	Tag *query = handleTag(stanzaTag);
	delete stanzaTag;

	bool ret = true;
	if (query) {
		Tag *tag = query->findChild("command");
		ret = (tag && tag->findAttribute("status") != "executing");
		Transport::instance()->send(query);
	}

	return ret;
}

