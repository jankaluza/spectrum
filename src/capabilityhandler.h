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

#ifndef CAPABILITY_HANDLER_H
#define CAPABILITY_HANDLER_H

#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include "purple.h"
#include "configfile.h"
#include "log.h"

#include <gloox/iqhandler.h>
#include <gloox/discohandler.h>

extern LogClass Log_;

typedef enum { 	GLOOX_FEATURE_ROSTERX = 2,
				GLOOX_FEATURE_XHTML_IM = 4,
				GLOOX_FEATURE_FILETRANSFER = 8,
				GLOOX_FEATURE_CHATSTATES = 16,
				GLOOX_FEATURE_TUNE = 32,
				GLOOX_FEATURE_MOOD = 64,
				GLOOX_FEATURE_ACTIVITY = 128,
				} GlooxImportantFeatures;

using namespace gloox;

struct Version {
	std::string version;
	std::string jid;
};

// Handler for disco#info stanzas.
class CapabilityHandler : public DiscoHandler {
	public:
		CapabilityHandler();
		~CapabilityHandler();

		// Handles disco#info.
		void handleDiscoInfo(const JID &jid, const Disco::Info &info, int context);
		void handleDiscoInfo(const JID &jid, Tag *query, int context);

		// Handles disco#items.
		void handleDiscoItems(const JID &jid, const Disco::Items &items, int context);

		// Handles disco erros.
		void handleDiscoError(const JID &jid, const Error *error, int context);

		// Setups handler for disco#info responce from `jid`, which should contain capabilities
		// known by `client`. Returns handler's descriptor.
		int waitForCapabilities(const std::string &client, const std::string &jid);
		
		// Returns true 
		bool hasVersion(int i);

	private:
		std::map <int, Version> m_versions;
		int m_nextVersion;
};

#endif
