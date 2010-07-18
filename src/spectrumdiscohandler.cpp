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

#include "spectrumdiscohandler.h"
#include "main.h"
#include <time.h>
#include <gloox/clientbase.h>
#include <gloox/tag.h>
#include <glib.h>
#include "usermanager.h"
#include "log.h"
#include "spectrum_util.h"

#include "sql.h"
#include <sstream>
#include <fstream>

SpectrumDiscoHandler::SpectrumDiscoHandler(GlooxMessageHandler *parent) : IqHandler() {
	m_parent = parent;
}

SpectrumDiscoHandler::~SpectrumDiscoHandler() {
}

bool SpectrumDiscoHandler::handleIq (const IQ &stanza) {
	if (stanza.to().username() == "" || stanza.subtype() == IQ::Set) {
		return m_parent->j->disco()->handleIq(stanza);
	}
	else {
		Tag *re = NULL;
		const Disco::Info *info = stanza.findExtension<Disco::Info>( ExtDiscoInfo );
		if (info) {
			if (!info->node().empty()) {
				Disco::IdentityList identities;
				StringList features;
				std::map<std::string, std::list<DiscoNodeHandler*> >::const_iterator it = m_nodeHandlers.find( info->node() );
				if (it == m_nodeHandlers.end()) {
					return false;
				}
				else {
					std::list<DiscoNodeHandler*>::const_iterator in = (*it).second.begin();
					for( ; in != (*it).second.end(); ++in ) {
						Disco::IdentityList il = (*in)->handleDiscoNodeIdentities( stanza.from(), info->node() );
						il.sort(); // needed on win32
						identities.merge( il );
						StringList fl = (*in)->handleDiscoNodeFeatures( stanza.from(), info->node() );
						fl.sort();  // needed on win32
						features.merge( fl );
						break;
					}
				}
				re = generateDiscoInfoResponse(stanza, identities, features);
			}
			else {
				Disco::IdentityList il;
				Disco::IdentityList::const_iterator it = m_identities.begin();
				for( ; it != m_identities.end(); ++it ) {
					il.push_back( new Disco::Identity( *(*it) ) );
				}
				re = generateDiscoInfoResponse(stanza, il, m_features);
			}
			if (!re)
				return false;
			m_parent->j->send(re);
			return true;
		}
		
		
	}
	return false;
}

void SpectrumDiscoHandler::handleIqID (const IQ &iq, int context) {
	m_parent->j->disco()->handleIqID(iq, context);
}

void SpectrumDiscoHandler::registerNodeHandler(DiscoNodeHandler* nh, const std::string& node) {
	m_nodeHandlers[node].push_back(nh);
}

Tag *SpectrumDiscoHandler::generateDiscoInfoResponse(const IQ &stanza, const Disco::IdentityList &identities, const StringList &features) {
	IQ re( IQ::Result, stanza.from(), stanza.id() );
	re.setFrom( stanza.to() );
	Tag *t = re.tag();

	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "http://jabber.org/protocol/disco#info");
	
	Disco::IdentityList::const_iterator it = identities.begin();
	for( ; it != identities.end(); ++it ) {
		query->addChild((*it)->tag());
	}
	
	for (StringList::const_iterator it = features.begin() ; it != features.end(); ++it ) {
		Tag *feature = new Tag("feature");
		feature->addAttribute("var", *it);
		query->addChild(feature);
	}
	
	t->addChild(query);
	
// <iq to='123@localhost/hanzz-laptop' from='icq.localhost' id='aac2a' type='result' xmlns='jabber:component:accept'><query xmlns='http://jabber.org/protocol/disco#info'><identity category='gateway' type='msn' name='Muj Transport'/><feature var='http://jabber.org/protocol/caps'/><feature var='http://jabber.org/protocol/chatstates'/><feature var='http://jabber.org/protocol/commands'/><feature var='http://jabber.org/protocol/commands'/><feature var='http://jabber.org/protocol/disco#info'/><feature var='http://jabber.org/protocol/disco#info'/><feature var='http://jabber.org/protocol/disco#items'/><feature var='http://jabber.org/protocol/si'/><feature var='http://jabber.org/protocol/si/profile/file-transfer'/><feature var='jabber:iq:gateway'/><feature var='jabber:iq:register'/><feature var='jabber:iq:version'/><feature var='urn:xmpp:ping'/><feature var='vcard-temp'/></query></iq>
	
	return t;
}
