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

#include "twitter.h"
#include "../main.h"

TwitterProtocol::TwitterProtocol(GlooxMessageHandler *main){
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

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

TwitterProtocol::~TwitterProtocol() {}

bool TwitterProtocol::isValidUsername(const std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> TwitterProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> TwitterProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string TwitterProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your Twitter username and password:");
	return "not defined";
}

bool TwitterProtocol::onPresenceReceived(AbstractUser *user, const Presence &stanza) {
	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
	Tag *stanzaTag = stanza.tag();
	if (stanza.to().username() != "") {
		if (user->isConnectedInRoom(stanza.to().username().c_str())) {
		}
		else if (isMUC && stanza.presence() != Presence::Unavailable) {
			if (user->isConnected()) {
				std::string name = JID(stanzaTag->findAttribute("to")).username();
				std::string nickname = JID(stanzaTag->findAttribute("to")).resource();
				PurpleChat *chat = purple_blist_find_chat(user->account(), "Timeline: my");
				if (chat == NULL) {
					delete stanzaTag;
					return false;
				}

				PurpleConnection *gc = purple_account_get_connection(user->account());
				user->setRoomResource(name, JID(stanzaTag->findAttribute("from")).resource());
				user->setRoomResource("timeline: home", JID(stanzaTag->findAttribute("from")).resource());
				std::cout << "joinchat " << purple_chat_get_components(chat) << "\n";
				serv_join_chat(gc, purple_chat_get_components(chat));
				
				
			}
		}
	}
	delete stanzaTag;
	return false;
}
