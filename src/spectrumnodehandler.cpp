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

#include "spectrumnodehandler.h"
#include "log.h"
#include "transport.h"

SpectrumNodeHandler::SpectrumNodeHandler() {
	m_features = Transport::instance()->protocol()->buddyFeatures();
	if (find(m_features.begin(), m_features.end(), "http://jabber.org/protocol/disco#items") == m_features.end())
		m_features.push_back("http://jabber.org/protocol/disco#items");
	if (find(m_features.begin(), m_features.end(), "http://jabber.org/protocol/disco#info") == m_features.end())
		m_features.push_back("http://jabber.org/protocol/disco#info");
	m_features.sort();
}

SpectrumNodeHandler::~SpectrumNodeHandler() {
	
}

StringList SpectrumNodeHandler::handleDiscoNodeFeatures (const JID &from, const std::string &node) {
	return m_features;
}

Disco::IdentityList SpectrumNodeHandler::handleDiscoNodeIdentities (const JID &from, const std::string &node) {
	Disco::IdentityList l;
	l.push_back( new Disco::Identity("client", "pc", "Spectrum") );
	return l;
}

Disco::ItemList SpectrumNodeHandler::handleDiscoNodeItems (const JID &from, const JID &to, const std::string &node) {
	Disco::ItemList lst;
	return lst;
}

