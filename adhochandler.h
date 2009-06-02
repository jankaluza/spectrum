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
 
#ifndef _HI_ADHOC_HANDLER_H
#define _HI_ADHOC_HANDLER_H

#include <string>
#include "account.h"
#include "user.h"
#include "glib.h"
#include "gloox/disconodehandler.h"

class GlooxMessageHandler;
class AdhocRepeater;

class GlooxAdhocHandler : public DiscoNodeHandler, public DiscoHandler, public IqHandler
{
	public:
		GlooxAdhocHandler(GlooxMessageHandler *m);
		~GlooxAdhocHandler();
		StringList handleDiscoNodeFeatures (const JID &from, const std::string &node);
		Disco::IdentityList handleDiscoNodeIdentities( const JID& jid, const std::string& node );
		Disco::ItemList handleDiscoNodeItems (const JID &from, const JID &to, const std::string &node=EmptyString);
		bool handleIq (const IQ &iq);
		void handleIqID (const IQ &iq, int context);
		void handleDiscoInfo(const JID &jid, const Disco::Info &info, int context);
		void handleDiscoItems(const JID &jid, const Disco::Items &items, int context);
		void handleDiscoError(const JID &jid, const Error *error, int context);
		void registerSession(const std::string &jid, AdhocRepeater *repeater) {std::cout << jid << "\n"; m_sessions[jid] = repeater; }
		void unregisterSession(std::string &jid) { m_sessions.erase(jid); }
		bool hasSession(const std::string &jid);

	private:
		GlooxMessageHandler *main;
		std::map<std::string, AdhocRepeater *> m_sessions;

};

#endif
 