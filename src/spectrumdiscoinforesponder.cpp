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

#include "spectrumdiscoinforesponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "transport.h"
#include "Swiften/Disco/DiscoInfoResponder.h"
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Swiften.h"

using namespace Swift;
using namespace boost;

SpectrumDiscoInfoResponder::SpectrumDiscoInfoResponder(Swift::IQRouter *router) : Swift::GetResponder<DiscoInfo>(router) {
	m_transportInfo.addIdentity(DiscoInfo::Identity(CONFIG().discoName, "gateway", PROTOCOL()->gatewayIdentity()));

	m_buddyInfo.addIdentity(DiscoInfo::Identity(CONFIG().discoName, "client", "pc"));

	std::list<std::string> features = PROTOCOL()->transportFeatures();
	features.sort();
	for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
		m_transportInfo.addFeature(*it);
	}

	std::list<std::string> f = PROTOCOL()->buddyFeatures();
	if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#items") == f.end())
		f.push_back("http://jabber.org/protocol/disco#items");
	if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#info") == f.end())
		f.push_back("http://jabber.org/protocol/disco#info");
	f.sort();
	for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
		m_buddyInfo.addFeature(*it);
	}

	CapsInfoGenerator caps("");
	CONFIG().caps = caps.generateCapsInfo(m_buddyInfo);
	
}

SpectrumDiscoInfoResponder::~SpectrumDiscoInfoResponder() {
	
}

bool SpectrumDiscoInfoResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::DiscoInfo> info) {
	if (!info->getNode().isEmpty()) {
		sendError(from, id, ErrorPayload::ItemNotFound, ErrorPayload::Cancel);
		return true;
	}

	// presence for transport
	if (to.getNode().isEmpty()) {
		sendResponse(from, id, boost::shared_ptr<DiscoInfo>(new DiscoInfo(m_transportInfo)));
	}
	// presence for buddy
	else {
		sendResponse(from, id, boost::shared_ptr<DiscoInfo>(new DiscoInfo(m_buddyInfo)));
	}
	return true;
}
