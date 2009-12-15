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

#include "spectrummucconversation.h"
#include "main.h" // just for replaceBadJidCharacters
#include "transport.h"
#include "usermanager.h"

SpectrumMUCConversation::SpectrumMUCConversation(PurpleConversation *conv, const std::string &jid) : AbstractConversation(SPECTRUM_CONV_GROUPCHAT), m_conv(conv) {
	m_jid = jid;
}

SpectrumMUCConversation::~SpectrumMUCConversation() {
}

void SpectrumMUCConversation::handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	std::string name(who);

	// send message to user
	std::string message(purple_unescape_html(msg));
	Message s(Message::Groupchat, m_jid, message);
	s.setFrom(m_jid + "/" + name);

	Transport::instance()->send( s.tag() );
}

void SpectrumMUCConversation::addUsers(AbstractUser *user, GList *cbuddies) {
	
}

void SpectrumMUCConversation::renameUser(AbstractUser *user, const char *old_name, const char *new_name, const char *new_alias) {
	
}

void SpectrumMUCConversation::removeUsers(AbstractUser *user, GList *users) {
	
}
