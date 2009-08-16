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

#ifndef _HI_MYSPACE_PROTOCOL_H
#define _HI_MYSPACE_PROTOCOL_H

#include "abstractprotocol.h"

class GlooxMessageHandler;
extern Localization localization;

class MyspaceProtocol : AbstractProtocol
{
	public:
		MyspaceProtocol(GlooxMessageHandler *main);
		~MyspaceProtocol();
		const std::string gatewayIdentity() { return "myspace"; }
		const std::string protocol() { return "prpl-myspace"; }
		bool isValidUsername(const std::string &username);
		void prepareUserName(std::string &username);
		std::list<std::string> transportFeatures();
		std::list<std::string> buddyFeatures();
		std::string text(const std::string &key);
		Tag *getVCardTag(User *user, GList *vcardEntries) { return NULL; }
		bool isMUC(User *user, const std::string &jid) { return false; }
// 		std::string notifyUsername() { return "info"; }
// 		std::string userSearchAction() { return "Search for buddies..."; }
		
		void onConnected(User *user);
// 		std::string userSearchColumn() { return "ID"; }
	
	private:
		GlooxMessageHandler *m_main;
		std::list<std::string> m_transportFeatures;
		std::list<std::string> m_buddyFeatures;
	
};

#endif	
	
