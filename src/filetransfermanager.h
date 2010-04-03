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
#include "filetransferrepeater.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <gloox/siprofilefthandler.h>
#include <gloox/siprofileft.h>
#include <gloox/jid.h>
#include <iostream>
#include <algorithm>

using namespace gloox;

struct Info {
	std::string filename;
	int size;
	bool straight;
};

// Manages and creates filetransfer requests.
// FiletransferManager::handleFTRequest(...) forwards filetransfer request to libpurple. Libpurple then
// should create PurpleXfer and Spectrum will create FiletransferRepe
class FileTransferManager : public gloox::SIProfileFTHandler {
	public:
		FileTransferManager();
		~FileTransferManager();

		// Sets SIProfileFT class used to accept/request filetransfers.
		void setSIProfileFT(gloox::SIProfileFT *sipft);

		// Handles incoming FT request.
		void handleFTRequest (const JID &from, const JID &to, const std::string &sid, const std::string &name, long size, const std::string &hash, const std::string &date, const std::string &mimetype, const std::string &desc, int stypes);

		// Handles incoming FT error.
		void handleFTRequestError (const IQ &iq, const std::string &sid) {}

		// Handles newly created Bytestream used to send/receive file.
		void handleFTBytestream (Bytestream *bs);

		// Handles newly created PurpleXfer.
		void handleXferCreated(PurpleXfer *xfer);

		// Handles file receive request.
		void handleXferFileReceiveRequest(PurpleXfer *xfer);

		// Called when file is received.
		void handleXferFileReceiveComplete(PurpleXfer *xfer);

		// Handles OOB request (http://xmpp.org/extensions/xep-0066.html).
		const std::string handleOOBRequestResult (const JID &from, const JID &to, const std::string &sid) { return ""; }

		// Sends file to user.
		void sendFile(const std::string &jid, const std::string &from, const std::string &name, const std::string &file);

		// Sets informations about filetransfer according to SID. Used when requesting filetransfer. If `straight`
		// is false, file will be stored on server and link will be sent (straight is false if receiver doesn't
		// support filetransfer).
		void setTransferInfo(const std::string &r_sid, const std::string &name, long size, bool straight) {
			m_info[r_sid].filename = name;
			m_info[r_sid].size = size;
			m_info[r_sid].straight = straight;
		}

		// Returns filetransfer info.
		const Info &getTransferInfo(const std::string &r_sid) { return m_info[r_sid]; }

	private:
		struct WaitingXferData {
			std::string sid;
			std::string from;
			long size;
		};

		gloox::SIProfileFT *m_sip;
		std::list<std::string> m_sendlist;
		std::map<std::string, Info> m_info;
		std::map<std::string, WaitingXferData> m_waitingForXfer;
		std::map<std::string, FiletransferRepeater *> m_repeaters;
};

PurpleXferUiOps * getXferUiOps();

#endif

