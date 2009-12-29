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

#ifndef FILETRANSFERMANAGER_H
#define FILETRANSFERMANAGER_H

#include "configfile.h"
#include "thread.h"
#include "purple.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <gloox/siprofilefthandler.h>
#include <gloox/siprofileft.h>
#include <gloox/jid.h>
#include <iostream>
#include <algorithm>

using namespace gloox;

class FileTransferManager : public gloox::SIProfileFTHandler {
	public:
		FileTransferManager();
		~FileTransferManager();

		void setSIProfileFT(gloox::SIProfileFT *sipft);
		void handleFTRequest (const JID &from, const JID &to, const std::string &sid, const std::string &name, long size, const std::string &hash, const std::string &date, const std::string &mimetype, const std::string &desc, int stypes);
		void handleFTRequestError (const IQ &iq, const std::string &sid) {}
		void handleFTBytestream (Bytestream *bs);
		const std::string handleOOBRequestResult (const JID &from, const JID &to, const std::string &sid) { return ""; }

		void sendFile(const std::string &jid, const std::string &from, const std::string &name, const std::string &file);
		void setInfo(const std::string &r_sid, const std::string &name, long size, bool straight) {
			m_info[r_sid].filename = name;
			m_info[r_sid].size = size;
			m_info[r_sid].straight = straight;
		}

	private:
		struct Info {
			std::string filename;
			int size;
			bool straight;
		};

		gloox::SIProfileFT *m_sip;
		std::list<std::string> m_sendlist;
		std::map<std::string, Info> m_info;
};
#endif

