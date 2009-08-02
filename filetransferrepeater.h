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

#ifndef _SPECTRUM_FILETRANSFER_REPEATER_H
#define _SPECTRUM_FILETRANSFER_REPEATER_H

#include "localization.h"
#include "gloox/tag.h"
#include "gloox/presence.h"
#include "gloox/siprofileft.h"
#include "conversation.h"
#include "ft.h"

class User;
extern Localization localization;

class GlooxMessageHandler;

using namespace gloox;

class FiletransferRepeater {
	
	public:
		FiletransferRepeater(GlooxMessageHandler *main, const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from);
		~FiletransferRepeater() {}
		
		void registerXfer(PurpleXfer *xfer);
	
	private:
		GlooxMessageHandler *m_main;
		JID m_to;
		std::string m_sid;
		SIProfileFT::StreamType m_type;
		JID m_from;
		PurpleXfer *m_xfer;
	
};

#endif
