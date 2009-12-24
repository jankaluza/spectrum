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

#include "autoconnectloop.h"
#include "main.h"
#include "log.h"
#include "usermanager.h"
#include "transport.h"

/*
 * Callback which is called periodically and restoring connections.
 */
static gboolean iter(gpointer data){
	AutoConnectLoop *loop = (AutoConnectLoop*) data;
	return loop->restoreNextConnection();
}

AutoConnectLoop::AutoConnectLoop() {
	m_users = Transport::instance()->sql()->getOnlineUsers();
	m_timer = new SpectrumTimer(1000, iter, this);
	m_timer->start();
	
	if (m_users.size() == 0)
		delete this;
}

AutoConnectLoop::~AutoConnectLoop() {
	Log("connection restorer", "Restorer deleted");
	delete m_timer;
}

bool AutoConnectLoop::restoreNextConnection() {
	if (m_users.size() == 0)
		return false;
	std::string jid = m_users.back();
	m_users.pop_back();

	Log("connection restorer", "Checking next jid " << jid);
	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(jid);
	if (user == NULL) {
		Log("connection restorer", "Sending probe presence to " << jid);
		Tag *stanza = new Tag("presence");
		stanza->addAttribute( "to", jid);
		stanza->addAttribute( "type", "probe");
		stanza->addAttribute( "from", Transport::instance()->jid());
		Transport::instance()->send(stanza);
		return true;
	}
	Log("connection restorer", "There is no other account to be checked => stopping Restorer");
	return false;
}
