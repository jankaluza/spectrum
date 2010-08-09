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

#ifndef _PROTOCOL_MANAGER_H
#define _PROTOCOL_MANAGER_H

#include <string>
#include "glib.h"
#include "protocols/abstractprotocol.h"

GList *getSupportedProtocols();

void addSupportedProtocol(void *data);

class _spectrum_protocol {
	public:
		_spectrum_protocol(const std::string &name, AbstractProtocol *(*fnc)());
		std::string prpl_id;
		AbstractProtocol *(*create_protocol)();
};

#define SPECTRUM_PROTOCOL(NAME, CLASS) static AbstractProtocol *create_##NAME() { \
		return (AbstractProtocol *) new CLASS(); \
	} \
	_spectrum_protocol _spectrum_protocol_##NAME(#NAME, &create_##NAME);
	

#endif
