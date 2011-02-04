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

enum {
	SORT_BY_JID,
	SORT_BY_UIN,
	SORT_BY_BUDDIES,
};

struct SortData {
	std::string data;
	int iKey;
	std::string sKey;
};

struct ListData {
	std::list<SortData> output;
	bool only_vip;
	bool show_jid;
	bool show_uin;
	bool show_buddies;
	int sort_by;
	unsigned int users_per_page;
};

static bool compareSDataASC(SortData &a, SortData &b) {
	return strcmp(a.sKey.c_str(), b.sKey.c_str()) < 0;
}

static bool compareIDataASC(SortData &a, SortData &b) {
	return a.iKey < b.iKey;
}

static bool compareSDataDESC(SortData &a, SortData &b) {
	return strcmp(a.sKey.c_str(), b.sKey.c_str()) > 0;
}

static bool compareIDataDESC(SortData &a, SortData &b) {
	return a.iKey > b.iKey;
}

static void generateOutput(gpointer key, gpointer v, gpointer data) {
	ListData *d = (ListData *) data;
	User *user = (User *) v;

	if (d->users_per_page == 0)
		return;

	if (d->only_vip && !user->isVIP())
		return;

	SortData output;
	d->users_per_page--;

	if (d->show_jid) {
		output.data += user->jid() + ";";
		if (d->sort_by == SORT_BY_JID) {
			output.sKey = user->jid();
		}
	}

	if (d->show_uin) {
		output.data += user->username() + ";";
		if (d->sort_by == SORT_BY_UIN) {
			output.sKey = user->username();
		}
	}

	if (d->show_buddies) {
		int buddies = user->buddiesCount();
		output.data += stringOf(buddies) + ";";
		if (d->sort_by == SORT_BY_BUDDIES) {
			output.iKey = buddies;
		}
	}
	output.data += "\n";
	d->output.push_back(output);
}

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
	values[tr(m_language.c_str(), _("List online users"))] = "List online users";
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
		else if (result == "List online users") {
			m_state = ADHOC_ADMIN_LIST_USERS;

			AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "executing");
			adhocTag->setTitle(tr(m_language.c_str(), _("List online users")));

			adhocTag->addFixedText(tr(m_language.c_str(), _("Which users should be selected?")));
			adhocTag->addBoolean(tr(m_language.c_str(), _("VIP users only")), "users_vip", false);

			adhocTag->addFixedText(tr(m_language.c_str(), _("What should be displayed?")));
			adhocTag->addBoolean(tr(m_language.c_str(), _("JID")), "show_jid", true);
			adhocTag->addBoolean(tr(m_language.c_str(), _("Legacy network username")), "show_uin", true);
			adhocTag->addBoolean(tr(m_language.c_str(), _("Number of buddies")), "show_buddies", true);
			
			adhocTag->addFixedText(tr(m_language.c_str(), _("Sorting")));
			std::map <std::string, std::string> values;
			values[tr(m_language.c_str(), _("JID"))] = "show_jid";
			values[tr(m_language.c_str(), _("Legacy network username"))] = "show_uin";
			values[tr(m_language.c_str(), _("Number of buddies"))] = "show_buddies";
			adhocTag->addListSingle(tr(m_language.c_str(), _("Sort by")), "show_sort_by", values);
			std::map <std::string, std::string> order;
			order[tr(m_language.c_str(), _("ASC"))] = "asc";
			order[tr(m_language.c_str(), _("DESC"))] = "desc";
			adhocTag->addListSingle(tr(m_language.c_str(), _("Sorting order")), "show_sort_order", order);
			adhocTag->addTextSingle(tr(m_language.c_str(), _("Users per page")), "show_max_per_page", "100");
			return adhocTag;
		}
	}
	else if (m_state == ADHOC_ADMIN_LIST_USERS) {
		if (!form.hasField("users_vip") || !form.hasField("show_jid") || !form.hasField("show_uin")
			|| !form.hasField("show_buddies") || !form.hasField("show_sort_by") || !form.hasField("show_sort_order")
			|| !form.hasField("show_max_per_page")
		) {
			std::cout << "error\n";
			return NULL;
		}

		ListData data;
		data.only_vip = form.field("users_vip")->value() == "1";
		data.show_jid = form.field("show_jid")->value() == "1";
		data.show_uin = form.field("show_uin")->value() == "1";
		data.show_buddies = form.field("show_buddies")->value() == "1";
		if (form.field("show_sort_by")->value() == "show_jid")
			data.sort_by = SORT_BY_JID;
		else if (form.field("show_sort_by")->value() == "show_uin")
			data.sort_by = SORT_BY_UIN;
		else if (form.field("show_sort_by")->value() == "show_buddies")
			data.sort_by = SORT_BY_BUDDIES;

// 		std::istringstream buffer(form.field("show_max_par_page")->value());
// 		buffer >> data.users_per_page;
		data.users_per_page = 100;

		bool sort_asc = form.field("show_sort_order")->value() == "asc";

		GHashTable *users = Transport::instance()->userManager()->getUsersTable();
		g_hash_table_foreach(users, generateOutput, &data);
		
		std::string output = std::string(data.show_jid ? "JID;" : "") + (data.show_uin ? "UIN;" : "") + (data.show_buddies ? "Number of buddies;" : "") + "\n";

		if (data.sort_by == SORT_BY_BUDDIES) {
			if (sort_asc)
				data.output.sort(compareIDataASC);
			else
				data.output.sort(compareIDataDESC);
		}
		else {
			if (sort_asc)
				data.output.sort(compareSDataASC);
			else
				data.output.sort(compareSDataDESC);
		}

		for (std::list<SortData>::iterator it = data.output.begin(); it != data.output.end(); it++) {
			output += (*it).data;
		}

		AdhocTag *adhocTag = new AdhocTag(tag->findAttribute("sessionid"), "transport_admin", "completed");
		adhocTag->addTextMulti("CSV:", "csv", output);
		return adhocTag;
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

