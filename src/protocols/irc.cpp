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
#include "../sql.h"
#include "../user.h"
#include "../adhoc/adhochandler.h"
#include "../adhoc/adhoctag.h"
#include "../transport.h"
#include "../spectrum_util.h"
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

IRCProtocol::IRCProtocol(GlooxMessageHandler *main){
	m_main = main;
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
	if (key == "instructions")
		return "Enter your Facebook email and password:";
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
		m_main->sql()->addSetting(user->storageId(), "nickserv", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("nickserv"), value);
	}
	user->setProtocolData(new IRCProtocolData());
}

void IRCProtocol::onConnected(AbstractUser *user) {
	IRCProtocolData *data = (IRCProtocolData *) user->protocolData();

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

	for (std::list <Tag*>::iterator it = data->autoConnectRooms.begin(); it != data->autoConnectRooms.end() ; it++ ) {
		Tag *stanza = (*it);
		GHashTable *comps = NULL;
		std::string name = JID(stanza->findAttribute("to")).username();
		std::string nickname = JID(stanza->findAttribute("to")).resource();

		PurpleConnection *gc = purple_account_get_connection(user->account());
		if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
			if (name.find("%") != std::string::npos)
				name = name.substr(0, name.find("%"));
			comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
		}
		if (comps) {
			user->setRoomResource(name, JID(stanza->findAttribute("from")).resource());
			serv_join_chat(gc, comps);
			g_hash_table_destroy(comps);
		}
		delete (*it);
	};

	data->autoConnectRooms.clear();
}

bool IRCProtocol::onPresenceReceived(AbstractUser *user, const Presence &stanza) {
	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
	Tag *stanzaTag = stanza.tag();
	if (stanza.to().username() != "") {
		IRCProtocolData *data = (IRCProtocolData *) user->protocolData();
		if (user->isConnectedInRoom(stanza.to().username().c_str())) {
		}
		else if (isMUC && stanza.presence() != Presence::Unavailable) {
			if (user->isConnected()) {
				GHashTable *comps = NULL;
				std::string name = JID(stanzaTag->findAttribute("to")).username();
				std::string nickname = JID(stanzaTag->findAttribute("to")).resource();

				PurpleConnection *gc = purple_account_get_connection(user->account());
				if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
					if (name.find("%") != std::string::npos)
						name = name.substr(0, name.find("%"));
					comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
				}
				if (comps) {
					user->setRoomResource(name, JID(stanzaTag->findAttribute("from")).resource());
					serv_join_chat(gc, comps);
				}
				
				
			}
			else {
				data->autoConnectRooms.push_back(stanza.tag());
			}
		}
	}
	delete stanzaTag;
	return false;
}


void IRCProtocol::onDestroy(AbstractUser *user) {
	IRCProtocolData *data = (IRCProtocolData *) user->protocolData();
	delete data;
}

void IRCProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	User *user = (User *) account->ui_data;
	const char *n = purple_value_get_string(user->getSetting("nickserv"));
	if (n) {
		purple_account_set_string(account, IRCHELPER_ID "_nickpassword", n);
	}
}

std::string IRCProtocol::prepareRoomName(AbstractUser *user, const std::string &room) {
	std::string name(room);
	if (name.find("%") == std::string::npos) {
		std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
		name = name + "@" + JID(user->username()).server();
	}
	return name;
}

std::string IRCProtocol::prepareName(AbstractUser *user, const JID &to) {
	std::string username = to.username();
	// "#pidgin%irc.freenode.org/HanzZ"
	if (!to.resource().empty() && to.resource() != "bot") {
		username = to.resource();
	}
	// "hanzz%irc.freenode.org/bot"
	else if (to.resource() == "bot") {
		size_t pos = username.find("%");
		if (pos != std::string::npos)
			username.erase((int) pos, username.length() - (int) pos);
	}
	std::for_each( username.begin(), username.end(), replaceJidCharacters() );
	return username;
}
