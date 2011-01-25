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

#ifndef _SPECTRUM_DISCO_HANDLER_H
#define _SPECTRUM_DISCO_HANDLER_H

#include "purple.h"
#include <gloox/stanza.h>
#include <gloox/stanzaextension.h>
#include <gloox/iqhandler.h>
#include <gloox/disconodehandler.h>
#include <gloox/disco.h>
#include "abstractconfiginterfacehandler.h"

using namespace gloox;


class SpectrumDiscoHandler : public IqHandler
{
	public:
		SpectrumDiscoHandler(Disco *parent);
		~SpectrumDiscoHandler();
		bool handleIq (const IQ &iq);
		void handleIqID (const IQ &iq, int context);

		void registerNodeHandler( DiscoNodeHandler* nh, const std::string& node );
		void setIdentity( const std::string& category, const std::string& type, const std::string& name = EmptyString ) {
			m_identities.push_back( new Disco::Identity( category, type, name ) );
		}
		void addFeature( const std::string& feature )
        { m_features.push_back( feature ); }

	private:
		Tag *generateDiscoInfoResponse(const IQ &stanza, const Disco::IdentityList &identities, const StringList &features);
		
		Disco *m_parent;
		std::map<std::string, std::list<DiscoNodeHandler*> > m_nodeHandlers;
		Disco::IdentityList m_identities;
		StringList m_features;
};

#endif
