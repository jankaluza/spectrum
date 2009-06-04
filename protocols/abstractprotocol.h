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

#ifndef _HI_ABSTRACTPROTOCOL_H
#define _HI_ABSTRACTPROTOCOL_H

#include <string>
#include <list>
#include "glib.h"
#include "gloox/tag.h"
#include "../user.h"

using namespace gloox;

class AbstractProtocol
{
	public:
		virtual ~AbstractProtocol() {}
		/*
		 * Returns gateway identity (http://xmpp.org/registrar/disco-categories.html)
		 */
		virtual std::string gatewayIdentity() = 0;
		/*
		 * Returns purple protocol name (for example "prpl-icq" for ICQ protocol)
		 */
		virtual std::string protocol() = 0;
		/*
		 * Returns true if the username is valid username for this protocol
		 */
		virtual bool isValidUsername(std::string &username) = 0;
		/*
		 * Returns revised username (for example for ICQ where username = "123- 456-789"; return "123456789";)
		 */
		virtual std::string prepareUserName(std::string &username) = 0;
		/*
		 * Returns disco features user by transport jid
		 */
		virtual std::list<std::string> transportFeatures() = 0;
		/*
		 * Returns disco features used by contacts
		 */
		virtual std::list<std::string> buddyFeatures() = 0;
		/*
		 * Returns pregenerated text according to key
		 */
		virtual std::string text(const std::string &key) = 0;
		/*
		 * Returns VCard Tag*
		 */
		virtual Tag *getVCardTag(User *user, GList *vcardEntries) = 0;
};

#endif
