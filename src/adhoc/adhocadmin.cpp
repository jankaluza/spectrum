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
#include "../usermanager.h"
#include "transport.h"
#include "../log.h"
#include "adhoctag.h"
#include "main.h"
#include "Poco/Format.h"
#include "registerhandler.h"
#include "adhochandler.h"

AdhocAdmin::AdhocAdmin(AbstractUser *user, const std::string &from, const std::string &id) {
	setRequestType(CALLER_ADHOC);
	m_from = std::string(from);
	m_state = ADHOC_ADMIN_INIT;
	
	if (user)
		m_language = std::string(user->getLang());
	else
		m_language = Transport::instance()->getConfiguration().language;

	AdhocTag *adhocTag = new AdhocTag(Transport::instance()->getId(), "transport_admin", "executing");
	adhocTag->setAction("next");
	adhocTag->setTitle(tr(m_language.c_str(), _("Transport administration")));
	adhocTag->setInstructions(tr(m_language.c_str(), _("Please select a configuration area.")));
	
	std::map <std::string, std::string> values;
	values[tr(m_language.c_str(), _("User"))] = "User";
	values[tr(m_language.c_str(), _("Send message to online users"))] = "Send message to online users";
	values[tr(m_language.c_str(), _("Register new user"))] = "Register new user";
	values[tr(m_language.c_str(), _("Unregister user"))] = "Unregister user";
	adhocTag->addListSingle(tr(m_language.c_str(), _("Command")), "config_area", values);

	GlooxAdhocHandler::sendAdhocResult(from, id, adhocTag);
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

AdhocTag *AdhocAdmin::handleAdhocTag(Tag *stanzaTag) {
	Tag *tag = stanzaTag->findChild("command");
	if (tag->hasAttribute("action", "cancel")) {
		return new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "canceled");
	}

	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x == NULL)
		return NULL;

	DataForm form(x);
	if (form.type() == TypeInvalid)
		return NULL;

	// Spectrumctl sends adhoc_state field to execute particular command from here
	if (form.hasField("adhoc_state")) {
		const std::string &data = form.field("adhoc_state")->value();
		if (data == "ADHOC_ADMIN_INIT")
			m_state = ADHOC_ADMIN_INIT;
		else if (data == "ADHOC_ADMIN_USER2")
			m_state = ADHOC_ADMIN_USER2;
		else if (data == "ADHOC_ADMIN_SEND_MESSAGE")
			m_state = ADHOC_ADMIN_SEND_MESSAGE;
		else if (data == "ADHOC_ADMIN_REGISTER_USER")
			m_state = ADHOC_ADMIN_REGISTER_USER;
		else if (data == "ADHOC_ADMIN_UNREGISTER_USER")
			m_state = ADHOC_ADMIN_UNREGISTER_USER;
	}

	if (m_state == ADHOC_ADMIN_INIT) {
		if (!form.hasField("config_area"))
			return NULL;

		const std::string &result = form.field("config_area")->value();
		if (result == "User") {
			m_state = ADHOC_ADMIN_USER;

			AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
			adhocTag->setAction("next");
			adhocTag->setTitle(tr(m_language.c_str(), _("User information")));
			adhocTag->setInstructions(tr(m_language.c_str(), _("Add bare JID of user you want to configure.")));
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Bare JID")), "user_jid");

			return adhocTag;
		}
		else if (result == "Send message to online users") {
			m_state = ADHOC_ADMIN_SEND_MESSAGE;

			AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
			adhocTag->setAction("complete");
			adhocTag->setTitle(tr(m_language.c_str(), _("Send message to online users")));
			adhocTag->setInstructions(tr(m_language.c_str(), _("Type message you want to send to all online users.")));
			adhocTag->addTextMulti(tr(m_language.c_str(), _("Message")), "message");

			return adhocTag;
		}
		else if (result == "Register new user") {
			m_state = ADHOC_ADMIN_REGISTER_USER;

			AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
			adhocTag->setAction("complete");
			adhocTag->setTitle(tr(m_language.c_str(), _("Register new user")));
			adhocTag->setInstructions(tr(m_language.c_str(), _("Enter information about new user.")));
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Bare JID")), "user_jid");
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Legacy Network username")), "user_username", "");
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Password")), "user_password", "");
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Language")), "user_language", Transport::instance()->getConfiguration().language);
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Encoding")), "user_encoding", Transport::instance()->getConfiguration().encoding);
			adhocTag->addBoolean(tr(m_language.c_str(), _("VIP")), "user_vip", false);

			return adhocTag;
		}
		else if (result == "Unregister user") {
			m_state = ADHOC_ADMIN_UNREGISTER_USER;

			AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
			adhocTag->setAction("complete");
			adhocTag->setTitle(tr(m_language.c_str(), _("Unregister user")));
			adhocTag->setInstructions(tr(m_language.c_str(), _("Enter bare JID of user to unregister.")));
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Bare JID")), "user_jid");

			return adhocTag;
		}
	}
	else if (m_state == ADHOC_ADMIN_USER) {
		m_state = ADHOC_ADMIN_USER2;
		if (!form.hasField("user_jid"))
			return NULL;
		const std::string &user_jid = form.field("user_jid")->value();

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
		
		return adhocTag;
	}
	else if (m_state == ADHOC_ADMIN_USER2) {
		if (!form.hasField("user_jid") || !form.hasField("user_vip"))
			return NULL;
		const std::string &user_jid = form.field("user_jid")->value();
		const std::string &user_vip = form.field("user_vip")->value();

		AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed");
		UserRow u = Transport::instance()->sql()->getUserByJid(user_jid);
		if (u.id != -1) {
			u.vip = user_vip == "1";
			Transport::instance()->sql()->updateUser(u);
		}
		else {
			adhocTag->addNote("error", tr(m_language.c_str(), _("This user is not registered.")));
		}

		return adhocTag;
	}
	else if (m_state == ADHOC_ADMIN_SEND_MESSAGE) {
		if (!form.hasField("message"))
			return NULL;
		
		std::string message = "";
		const StringList &values = form.field("message")->values();
		for (StringList::const_iterator it = values.begin(); it != values.end(); it++) {
			message+= (*it) + "\n";
		}

		Transport::instance()->userManager()->sendMessageToAll(message);
		return new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed");
	}
	else if (m_state == ADHOC_ADMIN_REGISTER_USER) {
		if (!form.hasField("user_jid") || !form.hasField("user_vip") || !form.hasField("user_password")
			|| !form.hasField("user_encoding") || !form.hasField("user_language") || !form.hasField("user_username"))
			return NULL;

		UserRow user;
		user.jid = form.field("user_jid")->value();
		user.vip = form.field("user_vip")->value() == "1";
		user.uin = form.field("user_username")->value();
		user.password = form.field("user_password")->value();
		user.encoding = form.field("user_encoding")->value();
		user.language = form.field("user_language")->value();

		AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed");
		
		if (!GlooxRegisterHandler::instance()->registerUser(user)) {
			adhocTag->addNote("error", tr(m_language.c_str(), _("This user is already registered.")));
		}

		return adhocTag;
	}
	else if (m_state == ADHOC_ADMIN_UNREGISTER_USER) {
		if (!form.hasField("user_jid"))
			return NULL;

		AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed");

		if (!GlooxRegisterHandler::instance()->unregisterUser(form.field("user_jid")->value())) {
			adhocTag->addNote("error", tr(m_language.c_str(), _("This user is not registered.")));
		}

		return adhocTag;
	}
	return NULL;
}

bool AdhocAdmin::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	AdhocTag *payload = handleAdhocTag(stanzaTag);
	delete stanzaTag;

	bool removeMe = true;
	if (payload) {
		removeMe = payload->isFinished();
		GlooxAdhocHandler::sendAdhocResult(stanza.from().full(), stanza.id(), payload);
	}

	return removeMe;
}

