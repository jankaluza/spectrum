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

#include "filetransfermanager.h"
#include "transport.h"
#include "filetransferrepeater.h"
#include "log.h"
#include "main.h"
#include "abstractuser.h"
#include "usermanager.h"
#include "spectrum_util.h"

FileTransferManager::FileTransferManager() {
}

FileTransferManager::~FileTransferManager() {
}

void FileTransferManager::setSIProfileFT(gloox::SIProfileFT *sipft) {
	m_sip = sipft;
}

void FileTransferManager::handleFTRequest (const JID &from, const JID &to, const std::string &sid, const std::string &name, long size, const std::string &hash, const std::string &date, const std::string &mimetype, const std::string &desc, int stypes) {
	Log("Received file transfer request from ", from.full() << " " << to.full() << " " << sid);

	std::string uname = to.username();
	std::for_each( uname.begin(), uname.end(), replaceJidCharacters() );

	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(from.bare());
	if (user && user->account() && user->isConnected()) {
		bool canSendFile = Transport::instance()->canSendFile(user->account(), uname);
		Log("CanSendFile = ", canSendFile);
		Log("filetransferWeb = ", Transport::instance()->getConfiguration().filetransferWeb);
		setTransferInfo(sid, name, size, canSendFile);

		user->addFiletransfer(from, sid, SIProfileFT::FTTypeS5B, to, size);
		// if we can't send file straightly, we just receive it and send link to buddy.
		if (canSendFile)
			serv_send_file(purple_account_get_connection(user->account()), uname.c_str(), name.c_str());
		else if (!Transport::instance()->getConfiguration().filetransferWeb.empty())
			m_sip->acceptFT(from, sid, SIProfileFT::FTTypeS5B, to);
		else
			m_sip->declineFT(from, sid, SIManager::RequestRejected , "There is no way how to transfer file.");
	}
}

void FileTransferManager::handleFTBytestream (Bytestream *bs) {
	Log("handleFTBytestream", "target: " << bs->target().full() << " initiator: " << bs->initiator().full());
	if (std::find(m_sendlist.begin(), m_sendlist.end(), bs->target().full()) == m_sendlist.end()) {
		std::string filename = "";
		// if we will store file, we have to replace invalid characters and prepare directories.
		if (m_info[bs->sid()].straight == false) {
			filename = m_info[bs->sid()].filename;
			// replace invalid characters
			for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
				if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
					*it = '_';
				}
			}
			std::string directory = Transport::instance()->getConfiguration().filetransferCache + "/" + generateUUID();
			purple_build_dir(directory.c_str(), 0755);
			filename = directory + "/" + filename;
		}
		AbstractUser *user = Transport::instance()->userManager()->getUserByJID(bs->initiator().bare());
		FiletransferRepeater *repeater = NULL;
		if (user) {
			Log("handleFTBytestream", "wants repeater " << bs->target().username());
			repeater = user->removeFiletransfer(bs->target().username());
			if (!repeater) return;
		}
		else {
			AbstractUser *user = Transport::instance()->userManager()->getUserByJID(bs->target().bare());
			if (!user)
				return;
			repeater = user->removeFiletransfer(bs->initiator().username());
			if (!repeater) return;
		}

		if (repeater->isSending())
			repeater->handleFTSendBytestream(bs, filename);
		else
			repeater->handleFTReceiveBytestream(bs, filename);
    } else {
        m_sendlist.erase(std::find(m_sendlist.begin(), m_sendlist.end(), bs->target().full()));
    }
    m_info.erase(bs->sid());
}

void FileTransferManager::sendFile(const std::string &jid, const std::string &from, const std::string &name, const std::string &file) {
    m_sendlist.push_back(jid);
    std::ifstream f;
    f.open(file.c_str(), std::ios::ate);
    if (f) {
        struct stat info;
        if (stat(file.c_str(), &info) != 0 ) {
            std::cout << "FileTransferManager::sendFile" << " stat() call failed!\n";
            return;
        }
		std::cout << "requesting filetransfer " << jid <<" " << file <<" as " << name << " " << info.st_size << "\n";
        std::string sid = m_sip->requestFT(jid, name, info.st_size, EmptyString, EmptyString, EmptyString, EmptyString, SIProfileFT::FTTypeAll, from);
        m_info[sid].filename = file;
        m_info[sid].size = info.st_size;
        f.close();
    } else {
        std::cout << "FileTransferManager::sendFile" << " Couldn't open the file " << file << "!\n";
    }
}
