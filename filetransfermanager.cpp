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

void FileTransferManager::setSIProfileFT(gloox::SIProfileFT *sipft,GlooxMessageHandler *parent) {
	m_sip = sipft;
	p = parent;
	mutex = new MyMutex();
}

void FileTransferManager::handleFTRequest (const JID &from, const JID &to, const std::string &sid, const std::string &name, long size, const std::string &hash, const std::string &date, const std::string &mimetype, const std::string &desc, int stypes, long offset, long length) {
	std::cout << "Received file transfer request from " << from.full() << " " << to.full() << " " << sid << ".\n";
	m_info[sid].filename = name;
	m_info[sid].size = size;
	m_sip->acceptFT(from, sid, SIProfileFT::FTTypeS5B, to);
}


void FileTransferManager::handleFTBytestream (Bytestream *bs) {
	if (std::find(m_sendlist.begin(), m_sendlist.end(), bs->target().full()) == m_sendlist.end()) {
		std::string filename = m_info[bs->sid()].filename;
		// replace invalid characters
		for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
			if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
				*it = '_';
			}
		} 
		filename=p->configuration().filetransferCache+"/"+bs->target().username()+"-"+p->j->getID()+"-"+filename;
		
		mutex->lock();
		m_progress[bs->sid()].filename=filename;
		m_progress[bs->sid()].incoming=true;
		m_progress[bs->sid()].state=0;
		m_progress[bs->sid()].user=bs->initiator().bare();
		m_progress[bs->sid()].to=bs->initiator();
		m_progress[bs->sid()].from=bs->target();
		m_progress[bs->sid()].stream=bs;
		std::cout << "FROM:" << bs->initiator().full() << " TO:" << bs->target().full();
		
		mutex->unlock();
		new ReceiveFile(bs,filename, m_info[bs->sid()].size,mutex,this);
    } else {
		// zatim to nepotrebujem u odchozich filu
// 		mutex->lock();
// 		m_progress[s5b->sid()].filename=m_info[s5b->sid()].filename;
// 		m_progress[s5b->sid()].incoming=false;
// 		m_progress[s5b->sid()].state=0;
// 		mutex->unlock();
        new SendFile(bs, m_info[bs->sid()].filename, m_info[bs->sid()].size,mutex,this);
        m_sendlist.erase(std::find(m_sendlist.begin(), m_sendlist.end(), bs->target().full()));
    }
    m_info.erase(bs->sid());
}

void FileTransferManager::sendFile(std::string jid, std::string from, std::string name, std::string file) {
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
