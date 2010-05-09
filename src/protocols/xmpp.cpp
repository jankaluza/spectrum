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

#include "xmpp.h"
#include "../main.h"

XMPPProtocol::XMPPProtocol(GlooxMessageHandler *main){
	m_main = main;
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("jabber:iq:gateway");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
// 	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

XMPPProtocol::~XMPPProtocol() {}

bool XMPPProtocol::isValidUsername(const std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> XMPPProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> XMPPProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string XMPPProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your Jabber ID and password:");
	return "not defined";
}

bool XMPPProtocol::onPresenceReceived(AbstractUser *user, const Presence &stanza) {
	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
	Tag *stanzaTag = stanza.tag();
	if (stanza.to().username() != "") {
		if (user->isConnectedInRoom(stanza.to().username().c_str())) {
		}
		else if (isMUC && stanza.presence() != Presence::Unavailable) {
			if (user->isConnected()) {
				GHashTable *comps = NULL;
				std::string name = JID(stanzaTag->findAttribute("to")).username();
				std::string nickname = JID(stanzaTag->findAttribute("to")).resource();

				PurpleConnection *gc = purple_account_get_connection(user->account());
				std::string name2;
				if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
					replace(name, "%", "@");
					replace(name, "#", "");
					name2 = name;
					name += "/" + nickname;
					comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
				}
				if (comps) {
					std::cout << "USERNAME: " << name2 << "\n";
					user->setRoomResource(name2, JID(stanzaTag->findAttribute("from")).resource());
					serv_join_chat(gc, comps);
				}
				
				
			}
		}
	}
	delete stanzaTag;
	return false;
}

std::string XMPPProtocol::prepareRoomName(AbstractUser *user, const std::string &room) {
// 	std::string r = room;
// 	replace(r, "@", "%", 1);
	return room;
}

// std::string XMPPProtocol::prepareName(AbstractUser *user, const JID &to) {
// 	std::string username = to.username();
// 	return username;
// }

void XMPPProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	std::string jid(purple_account_get_username(account));
	if (JID(jid).server() == "chat.facebook.com")
		purple_account_set_bool(account, "require_tls", false);
}
