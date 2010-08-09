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

#include "irc.h"
#include "../main.h"
#include "../usermanager.h"
#include "../user.h"
#include "../adhoc/adhochandler.h"
#include "../adhoc/adhoctag.h"
#include "../transport.h"
#include "../spectrum_util.h"
#include "../abstractbackend.h"
#include "cmds.h"

#define IRCHELPER_ID "core-rlaager-irchelper"

ConfigHandler::ConfigHandler(AbstractUser *user, const std::string &from, const std::string &id) : m_from(from), m_user(user) {
	setRequestType(CALLER_ADHOC);
	std::string bare(JID(from).bare());

	IQ _response(IQ::Result, from, id);
	Tag *response = _response.tag();
	response->addAttribute("from", Transport::instance()->jid());

	AdhocTag *adhocTag = new AdhocTag(Transport::instance()->getId(), "transport_irc_config", "executing");
	adhocTag->setAction("complete");
	adhocTag->setTitle("IRC Nickserv password configuration");
	adhocTag->setInstructions("Choose the server you want to change password for.");
	
	std::map <std::string, std::string> values;
	std::map<std::string, UserRow> users = Transport::instance()->sql()->getUsersByJid(bare);
	for (std::map<std::string, UserRow>::iterator it = users.begin(); it != users.end(); it++) {
		std::string server = (*it).second.jid.substr(bare.size());
		values[server] = stringOf((*it).second.id);
		m_userId.push_back(stringOf((*it).second.id));
	}
	adhocTag->addListSingle("IRC server", "irc_server", values);

	adhocTag->addTextPrivate("New NickServ password", "password");

	response->addChild(adhocTag);
	Transport::instance()->send(response);
}

ConfigHandler::~ConfigHandler() { }

bool ConfigHandler::handleIq(const IQ &stanza) {
	AdhocTag cmd(stanza);

	Tag *response = cmd.generateResponse();
	if (cmd.isCanceled()) {
		Transport::instance()->send(response);
		return true;
	}
	
	std::string serverId = cmd.getValue("irc_server");
	std::string password = cmd.getValue("password");

	if (serverId != "")
		Transport::instance()->sql()->updateSetting(atoi(serverId.c_str()), "nickserv", password);

	Transport::instance()->send(response);
	return true;
}

static AdhocCommandHandler * createConfigHandler(AbstractUser *user, const std::string &from, const std::string &id) {
	AdhocCommandHandler *handler = new ConfigHandler(user, from, id);
	return handler;
}

IRCProtocol::IRCProtocol() {
// 	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");
	m_transportFeatures.push_back("http://jabber.org/protocol/muc");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");
	
	adhocCommand command = { "IRC Nickserv password configuration", false, createConfigHandler };
	GlooxAdhocHandler::instance()->registerAdhocCommandHandler("transport_irc_config", command);
	
	m_irchelper = purple_plugins_find_with_id(IRCHELPER_ID);
	if (m_irchelper) {
		if (!purple_plugin_load(m_irchelper))
			m_irchelper = NULL;
	}
	std::cout << "IRCHELPER " << m_irchelper << "\n";

}

IRCProtocol::~IRCProtocol() {}

std::list<std::string> IRCProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> IRCProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string IRCProtocol::text(const std::string &key) {
	return "not defined";
}

Tag *IRCProtocol::getVCardTag(AbstractUser *user, GList *vcardEntries) {
// 	PurpleNotifyUserInfoEntry *vcardEntry;

	Tag *vcard = new Tag( "vCard" );
	vcard->addAttribute( "xmlns", "vcard-temp" );

	return vcard;
}

bool IRCProtocol::changeNickname(const std::string &nick, PurpleConversation *conv) {
	char *error = NULL;
	purple_cmd_do_command(conv, std::string("nick " + nick).c_str(), std::string("nick " + nick).c_str(), &error);
	if (error)
		g_free(error);
	return true;
}

void IRCProtocol::onUserCreated(AbstractUser *user) {
	PurpleValue *value;
	if ( (value = user->getSetting("nickserv")) == NULL ) {
		Transport::instance()->sql()->addSetting(user->storageId(), "nickserv", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("nickserv"), value);
	}
}

void IRCProtocol::onConnected(AbstractUser *user) {
	// IRC helper is not loaded, so we have to authenticate by ourself
	if (m_irchelper == NULL) {
		const char *n = purple_value_get_string(user->getSetting("nickserv"));
		std::string nickserv(n ? n : "");
		if (!nickserv.empty()) {
			std::string msg = "identify " + nickserv;
			PurpleConversation *conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, user->account(), "NickServ");
			purple_conv_chat_send(PURPLE_CONV_CHAT(conv), msg.c_str());
			purple_conversation_destroy(conv);
		}
	}
}


void IRCProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	User *user = (User *) account->ui_data;
	const char *n = purple_value_get_string(user->getSetting("nickserv"));
	if (n) {
		purple_account_set_string(account, IRCHELPER_ID "_nickpassword", n);
	}
}

void IRCProtocol::makePurpleUsernameRoom(AbstractUser *user, const JID &to, std::string &name) {
	std::string username = to.username();
	// "#pidgin%irc.freenode.org@irc.spectrum.im/HanzZ" -> "HanzZ"
	if (!to.resource().empty() && to.resource() != "bot") {
		username = to.resource();
	}
	// "hanzz%irc.freenode.org@irc.spectrum.im/bot" -> "hanzz"
	else if (to.resource() == "bot") {
		size_t pos = username.find("%");
		if (pos != std::string::npos)
			username.erase((int) pos, username.length() - (int) pos);
	}
	// "#pidgin%irc.freenode.org@irc.spectrum.im" -> #pidgin
	else {
		size_t pos = username.find("%");
		if (pos != std::string::npos)
			username.erase((int) pos, username.length() - (int) pos);
	}
	std::for_each( username.begin(), username.end(), replaceJidCharacters() );
	name.assign(username);
}

void IRCProtocol::makePurpleUsernameIM(AbstractUser *user, const JID &to, std::string &name) {
	makePurpleUsernameRoom(user, to, name);
}

void IRCProtocol::makeRoomJID(AbstractUser *user, std::string &name) {
	// #pidgin" -> "#pidgin%irc.freenode.net@irc.spectrum.im"
	std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
	std::string name_safe = name;
	std::for_each( name_safe.begin(), name_safe.end(), replaceBadJidCharacters() ); // OK
	name.assign(name_safe + "%" + JID(user->username()).server() + "@" + Transport::instance()->jid());
	std::cout << "ROOMJID: " << name << "\n";
}

void IRCProtocol::makeUsernameRoom(AbstractUser *user, std::string &name) {
}

SPECTRUM_PROTOCOL(irc, IRCProtocol)
