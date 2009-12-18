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

#ifndef _ABSTRACT_BACKEND_H
#define _ABSTRACT_BACKEND_H

#include <string>
#include <list>
#include "account.h"
#include "value.h"

class AbstractBackend
{
	public:
		virtual ~AbstractBackend() {}
		virtual void addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type) = 0;
		virtual long addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group = "Buddies", const std::string &nickname = "") = 0;
		virtual GHashTable *getBuddies(long userId, PurpleAccount *account) = 0;

};

#endif
