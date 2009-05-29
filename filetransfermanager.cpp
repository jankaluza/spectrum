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
/*
void FileTransferManager::handleFTRequest( const gloox::JID &from,const gloox::JID &to, const std::string &id, const std::string &sid,
                                             const std::string &name, long size, const std::string &hash,
                                             const std::string &date, const std::string &mimetype,
                                             const std::string &desc, int stypes, long offset, long length) {
     std::cout << "Received file transfer request from " << from.full() << ".\n";
     m_info[sid].filename = name;
     m_info[sid].size = size;
     m_sip->acceptFT( from, to, id, gloox::SIProfileFT::FTTypeS5B );
}

void FileTransferManager::handleFTRequestError(gloox::Stanza *stanza, const std::string &sid) {
//     Log::i().log(LogStageError, "FileTransferManager::handleFTRequestError", "Received file transfer error from " + stanza->findAttribute("from") + ".");
//     Log::i().log(LogStageError, "FileTransferManager::handleFTRequestError", stanza->xml());
}

void FileTransferManager::handleFTSOCKS5Bytestream(gloox::SOCKS5Bytestream *s5b) {
//     Log::i().log(LogStageDebug, "FileTransferManager::handleFTSOCKS5Bytestream", "Incomming Bytestream.");


	if (std::find(m_sendlist.begin(), m_sendlist.end(), s5b->target().full()) == m_sendlist.end()) {


		std::string filename = m_info[s5b->sid()].filename;
		
// 		std::string filename;
	
// 		filename.resize(tempname.size());
	
		// replace invalid characters
		for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
			if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
				*it = '_';
			}
		} 
		filename=p->configuration().filetransferCache+"/"+s5b->target().username()+"-"+p->j->getID()+"-"+filename;
		
		mutex->lock();
		m_progress[s5b->sid()].filename=filename;
		m_progress[s5b->sid()].incoming=true;
		m_progress[s5b->sid()].state=0;
		m_progress[s5b->sid()].user=s5b->from().bare();
		m_progress[s5b->sid()].to=s5b->from();
		m_progress[s5b->sid()].from=s5b->target();
		m_progress[s5b->sid()].stream=s5b;
		std::cout << "FROM:" << s5b->initiator().full();
		
		mutex->unlock();
        new ReceiveFile(s5b,filename, m_info[s5b->sid()].size,mutex,this);
    } else {
		// zatim to nepotrebujem u odchozich filu
// 		mutex->lock();
// 		m_progress[s5b->sid()].filename=m_info[s5b->sid()].filename;
// 		m_progress[s5b->sid()].incoming=false;
// 		m_progress[s5b->sid()].state=0;
// 		mutex->unlock();
        new SendFile(s5b, m_info[s5b->sid()].filename, m_info[s5b->sid()].size,mutex,this);
        m_sendlist.erase(std::find(m_sendlist.begin(), m_sendlist.end(), s5b->target().full()));
    }
    m_info.erase(s5b->sid());
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
        std::string sid = m_sip->requestFT(jid,from,name, file, info.st_size);
        m_info[sid].filename = file;
        m_info[sid].size = info.st_size;
        f.close();
    } else {
        std::cout << "FileTransferManager::sendFile" << " Couldn't open the file " << file << "!\n";
    }
}*/
