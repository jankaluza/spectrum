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

Tag *IRCProtocol::getVCardTag(User *user, GList *vcardEntries) {
// 	PurpleNotifyUserInfoEntry *vcardEntry;

	Tag *vcard = new Tag( "vCard" );
	vcard->addAttribute( "xmlns", "vcard-temp" );

	return vcard;
}

void IRCProtocol::onUserCreated(User *user) {
	PurpleValue *value;
	if ( (value = user->getSetting("nickserv")) == NULL ) {
		m_main->sql()->addSetting(user->userKey(), "nickserv", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("nickserv"), value);
	}
}

void IRCProtocol::onConnected(User *user) {
	std::string nickserv(purple_value_get_string(user->getSetting("nickserv")));
	if (!nickserv.empty()) {
		Message msg(Message::Chat, JID("NickServ@server.cz"), "identify " + nickserv);
		msg.setFrom(user->jid());
		user->receivedMessage(msg);
	}
}

