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

#include "gg.h"
#include "../main.h"

GGProtocol::GGProtocol(GlooxMessageHandler *main){
	m_main = main;
	m_transportFeatures.push_back("jabber:iq:register");
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

GGProtocol::~GGProtocol() {}

bool GGProtocol::isValidUsername(const std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> GGProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> GGProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string GGProtocol::text(const std::string &key) {
	if (key == "instructions")
		return "Enter your GG number and password:";
	return "not defined";
}
