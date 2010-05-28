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

#include "messagesender.h"
#include "log.h"
#include "transport.h"
#include "spectrumtimer.h"
#include "gloox/message.h"

using namespace gloox;

static gboolean sendMessageCallback(void *data) {
	MessageSender *parent = (MessageSender *) data;
	return parent->_sendMessageToNext();
}

MessageSender::MessageSender() {
	m_sendMessage = new SpectrumTimer(100, sendMessageCallback, this);
}

MessageSender::~MessageSender(){
	delete m_sendMessage;
}

void MessageSender::addRecipient(const std::string &jid) {
	m_sendMessageUsers.push_back(jid);
}

bool MessageSender::isSending() {
	// If timer is active, we're sending messages...
	return m_sendMessage->isRunning();
}

bool MessageSender::sendMessage(const std::string &message) {
	if (isSending())
		return false;
	m_message = message;
	m_sendMessage->start();
	return true;
}

bool MessageSender::_sendMessageToNext() {
	if (m_sendMessageUsers.empty()) {
		m_message = "";
		return false;
	}
	Message s(Message::Chat, m_sendMessageUsers.back(), m_message);
	m_sendMessageUsers.pop_back();
	s.setFrom(Transport::instance()->jid());
	Transport::instance()->send(s.tag());
	return true;
}
