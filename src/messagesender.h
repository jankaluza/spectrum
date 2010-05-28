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

#ifndef SPECTRUM_MESSAGE_SENDER_H
#define SPECTRUM_MESSAGE_SENDER_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"

class SpectrumTimer;

// Sends message to all JIDs in list.
class MessageSender
{
	public:
		MessageSender();
		virtual ~MessageSender();

		// Adds new recipient
		void addRecipient(const std::string &jid);
		
		// TODO: Removes recepient.
		void removeRecepient() {}
		
		// Sends message to all users from. Return true if message can be,
		// and will be, sent.
		bool sendMessage(const std::string &message);

		// Return true if message is still beeing sent.
		bool isSending();

		// Used by SpectrumTimer. Don't call this function.
		bool _sendMessageToNext();

	private:
		std::string m_message;
		SpectrumTimer *m_sendMessage;
		std::list <std::string> m_sendMessageUsers;

};

#endif
