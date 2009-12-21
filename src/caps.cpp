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

#include "caps.h"
#include <gloox/clientbase.h>
#include <glib.h>
#include "transport.h"
#include "sql.h"
#include "usermanager.h"
#include "user.h"
#include "protocols/abstractprotocol.h"
#include "log.h"

GlooxDiscoHandler::GlooxDiscoHandler() : DiscoHandler() {
	m_nextVersion = 0;
}

GlooxDiscoHandler::~GlooxDiscoHandler(){
}

bool GlooxDiscoHandler::hasVersion(int name){
	return m_versions.find(name) != m_versions.end();
}

int GlooxDiscoHandler::waitForCapabilities(const std::string &client, const std::string &jid) {
	m_nextVersion++;
	m_versions[m_nextVersion].version = client;
	m_versions[m_nextVersion].jid = jid;
	return m_nextVersion;
}

void GlooxDiscoHandler::handleDiscoInfo(const JID &jid, const Disco::Info &info, int context) {
	if (!hasVersion(context))
		return;
	Tag *query = info.tag();
	if (query->findChild("identity") && !query->findChildWithAttrib("category","client")) {
		delete query;
		return;
	}

	int capabilities = 0;

	std::list<Tag*> features = query->findChildren("feature");
	for (std::list<Tag*>::const_iterator it = features.begin(); it != features.end(); ++it) {
		if ((*it)->findAttribute("var") == "http://jabber.org/protocol/rosterx") {
			capabilities |= GLOOX_FEATURE_ROSTERX;
		}
		else if ((*it)->findAttribute("var") == "http://jabber.org/protocol/xhtml-im") {
			capabilities |= GLOOX_FEATURE_XHTML_IM;
		}
		else if ((*it)->findAttribute("var") == "http://jabber.org/protocol/si/profile/file-transfer") {
			capabilities |= GLOOX_FEATURE_FILETRANSFER;
		}
		else if ((*it)->findAttribute("var") == "http://jabber.org/protocol/chatstates") {
			capabilities |= GLOOX_FEATURE_CHATSTATES;
		}
	}
	
	Log("*** FEATURES ARRIVED: ", capabilities);
	Transport::instance()->setClientCapabilities(m_versions[context].version, capabilities);

	JID j(m_versions[context].jid);
	AbstractUser *user;
	if (Transport::instance()->protocol()->isMUC(NULL, j.bare())) {
		std::string server = j.username().substr(j.username().find("%") + 1, j.username().length() - j.username().find("%"));
		user = (AbstractUser *) Transport::instance()->userManager()->getUserByJID(jid.bare() + server);
	}
	else {
		user = (AbstractUser *) Transport::instance()->userManager()->getUserByJID(jid.bare());
	}
	if (user && user->hasResource(jid.resource())) {
		if (user->getResource(jid.resource()).caps == -1) {
			user->setResource(jid.resource(), -256, capabilities);
			if (user->readyForConnect()) {
				user->connect();
			}
		}
	}
	// TODO: CACHE CAPS IN DATABASE ACCORDING TO VERSION
	m_versions.erase(context);
	delete query;
}

void GlooxDiscoHandler::handleDiscoItems(const JID &jid, const Disco::Items &items, int context) {
}

void GlooxDiscoHandler::handleDiscoError(const JID &jid, const Error *error, int context) {
}
