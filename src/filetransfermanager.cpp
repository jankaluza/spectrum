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
#include "usermanager.h"
#include "filetransferrepeater.h"
#include "log.h"
#include "main.h"

void FileTransferManager::setSIProfileFT(gloox::SIProfileFT *sipft,GlooxMessageHandler *parent) {
	m_sip = sipft;
	p = parent;
	mutex = new MyMutex();
}

void FileTransferManager::handleFTRequest (const JID &from, const JID &to, const std::string &sid, const std::string &name, long size, const std::string &hash, const std::string &date, const std::string &mimetype, const std::string &desc, int stypes) {
	std::cout << "Received file transfer request from " << from.full() << " " << to.full() << " " << sid << ".\n";
	m_info[sid].filename = name;
	m_info[sid].size = size;
	std::string uname = to.username();
	std::for_each( uname.begin(), uname.end(), replaceJidCharacters() );
	User *user = p->userManager()->getUserByJID(from.bare());
	if (user) {
		if (user->account()){
			if (user->isConnected()){
				bool send = false;
				PurplePlugin *prpl = NULL;
				PurplePluginProtocolInfo *prpl_info = NULL;
				PurpleConnection *gc = purple_account_get_connection(user->account());

				if(gc)
					prpl = purple_connection_get_prpl(gc);

				if(prpl)
					prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

				if (prpl_info && prpl_info->send_file) {
					if (!prpl_info->can_receive_file || prpl_info->can_receive_file(gc, uname.c_str())) {
						send = true;
					}
				}
				if (send) {
					user->addFiletransfer(from, sid, SIProfileFT::FTTypeS5B, to, size);
					serv_send_file(purple_account_get_connection(user->account()), uname.c_str(), name.c_str());
					m_info[sid].straight = true;
				}
				else {
					user->addFiletransfer(from, sid, SIProfileFT::FTTypeS5B, to, size);
					m_sip->acceptFT(from, sid, SIProfileFT::FTTypeS5B, to);
					m_info[sid].straight = false;
				}
			}
		}
	}
}


void FileTransferManager::handleFTBytestream (Bytestream *bs) {
	Log().Get("a") << "handleFTBytestream";
	if (std::find(m_sendlist.begin(), m_sendlist.end(), bs->target().full()) == m_sendlist.end()) {
		std::string filename = "";
		if (m_info[bs->sid()].straight == false) {
			filename = m_info[bs->sid()].filename;
			// replace invalid characters
			for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
				if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
					*it = '_';
				}
			}
			filename = p->configuration().filetransferCache + "/" + bs->target().username() + "-" + p->j->getID() + "-" + filename;
		}
		User *user = p->userManager()->getUserByJID(bs->initiator().bare());
		Log().Get("a") << "wants user" << bs->initiator().bare();
		FiletransferRepeater *repeater = NULL;
		if (user) {
			Log().Get("a") << "wants repeater" << bs->target().username();
			repeater = user->removeFiletransfer(bs->target().username());
			if (!repeater) return;
		}
		else {
			User *user = p->userManager()->getUserByJID(bs->target().bare());
			if (!user)
				return;
			repeater = user->removeFiletransfer(bs->initiator().username());
			if (!repeater) return;
		}

		if (repeater->isSending())
			repeater->handleFTSendBytestream(bs, filename);
		else
			repeater->handleFTReceiveBytestream(bs, filename);
// 		}
    } else {
		// zatim to nepotrebujem u odchozich filu
// 		mutex->lock();
// 		m_progress[s5b->sid()].filename=m_info[s5b->sid()].filename;
// 		m_progress[s5b->sid()].incoming=false;
// 		m_progress[s5b->sid()].state=0;
// 		mutex->unlock();
//         new SendFile(bs, m_info[bs->sid()].filename, m_info[bs->sid()].size,mutex,this);
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
