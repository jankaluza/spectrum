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

#include "protocolmanager.h"

static GList *supportedProtocols = NULL;

GList *getSupportedProtocols() {
	return supportedProtocols;
}

void addSupportedProtocol(void *data) {
	supportedProtocols = g_list_prepend(supportedProtocols, data);
}

_spectrum_protocol::_spectrum_protocol(const std::string &name, AbstractProtocol *(*fnc)()) {
	addSupportedProtocol(this);
	prpl_id = name;
	create_protocol = fnc;
}
