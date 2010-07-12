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
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/disconodehandler.h"
#include "gloox/iqhandler.h"
#include "gloox/discohandler.h"

using namespace gloox;

class AdhocRepeater;
class AdhocCommandHandler;
class AbstractUser;

/*
 * Structure used for registering Ad-Hoc command
 */
struct adhocCommand {
	std::string name;	// command's name
	bool admin;			// true if the user has to be admin to execute this command
	// function which creates AdhocCommandHandler class which will be used as handler for this command
	AdhocCommandHandler * (*createHandler)(AbstractUser *user, const std::string &from, const std::string &id);
};

class GlooxAdhocHandler : public DiscoNodeHandler, public DiscoHandler, public IqHandler
{
	public:
		GlooxAdhocHandler();
		~GlooxAdhocHandler();
		static GlooxAdhocHandler *instance() { return m_pInstance; }
		StringList handleDiscoNodeFeatures (const JID &from, const std::string &node);
		Disco::IdentityList handleDiscoNodeIdentities( const JID& jid, const std::string& node );
		Disco::ItemList handleDiscoNodeItems (const JID &from, const JID &to, const std::string &node=EmptyString);
		bool handleIq (const IQ &iq);
		void handleIqID (const IQ &iq, int context);
		void handleDiscoInfo(const JID &jid, const Disco::Info &info, int context);
		void handleDiscoItems(const JID &jid, const Disco::Items &items, int context);
		void handleDiscoError(const JID &jid, const Error *error, int context);
		void registerSession(const std::string &jid, AdhocCommandHandler *handler) {m_sessions[jid] = handler; }
		void unregisterSession(const std::string &jid);
		bool hasSession(const std::string &jid);
		void registerAdhocCommandHandler(const std::string &name, const adhocCommand command);

	private:
		std::map<std::string, AdhocCommandHandler *> m_sessions;	// sessions (m_sessions[full_jid] = handler)
		std::map<std::string, adhocCommand> m_handlers;				// handlers (m_handlers[node] = handler)
		static GlooxAdhocHandler *m_pInstance;
		std::map <std::string, int> m_nodes;

};

#endif
