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
#include "transport.h"
#include <sstream>
#include <fstream>

SpectrumDiscoHandler::SpectrumDiscoHandler(Disco *parent) : IqHandler() {
	m_parent = parent;
}

SpectrumDiscoHandler::~SpectrumDiscoHandler() {
}

bool SpectrumDiscoHandler::handleIq (const IQ &stanza) {
	if (stanza.to().username() == "" || stanza.subtype() == IQ::Set) {
		const Disco::Items *items = stanza.findExtension<Disco::Items>( ExtDiscoItems );
		if (items && items->node().empty()) {
			IQ re( IQ::Result, stanza.from(), stanza.id() );
			re.setFrom( stanza.to() );
			Tag *t = re.tag();

			Tag *query = new Tag("query");
			query->addAttribute("xmlns", "http://jabber.org/protocol/disco#items");
			Tag *item = new Tag("item");
			item->addAttribute("jid", Transport::instance()->jid());
			item->addAttribute("node", "http://jabber.org/protocol/commands");
			const char *_language = Transport::instance()->getConfiguration().language.c_str();
			item->addAttribute("name", tr(_language, _("Ad-Hoc Commands")));
			query->addChild(item);

			t->addChild(query);
			Transport::instance()->send(t);
			return true;
		}
		else {
			return m_parent->handleIq(stanza);
		}
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
			Transport::instance()->send(re);
			return true;
		}
		
		
	}
	return false;
}

void SpectrumDiscoHandler::handleIqID (const IQ &iq, int context) {
	m_parent->handleIqID(iq, context);
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
		delete (*it);
	}
	
	for (StringList::const_iterator it = features.begin() ; it != features.end(); ++it ) {
		Tag *feature = new Tag("feature");
		feature->addAttribute("var", *it);
		query->addChild(feature);
	}
	
	t->addChild(query);
	
	return t;
}
