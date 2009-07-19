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

#include "receivefile.h"
#include "log.h"
#include "main.h"
#include "filetransfermanager.h"
#include "usermanager.h"

static gboolean transferFinished(gpointer data){
	FileTransferManager *d = (FileTransferManager*) data;
	Log().Get("gloox") << "trying to send file to legacy network";
	if (d){
		d->mutex->lock();
		for(std::map<std::string, progress>::iterator u = d->m_progress.begin(); u != d->m_progress.end();){
			if ((*u).second.state==1){
				User *user = d->p->userManager()->getUserByJID((*u).second.user);
				if (user){
					Log().Get(user->jid()) << "trying to send file "<< (*u).second.filename <<" to " << (*u).second.from.username();
					if (user->account()){
						if (user->isConnected()){
							if (user->isVIP()){
								Log().Get(user->jid()) << "sending file";
								serv_send_file(purple_account_get_connection(user->account()),(*u).second.from.username().c_str(),(*u).second.filename.c_str());
							}
							Log().Get(user->jid()) << "sending download message";
							std::string basename(g_path_get_basename((*u).second.filename.c_str()));
							if (user->isVIP()) {
								user->p->sql()->addDownload(basename,"1");
							}
							else {
								user->p->sql()->addDownload(basename,"0");
							}
							Message s(Message::Chat, (*u).second.from.bare(), "Uzivatel Vam poslal soubor '"+basename+"'. Muzete jej stahnout z adresy http://soumar.jabbim.cz/icq/" + basename +" .");
							s.setFrom(user->jid());
							user->receivedMessage(s);
						}
					}
				}
				(*u).second.state=2;
				Log().Get("gloox") << "disposing this stream";
				d->m_sip->dispose((*u).second.stream);
				d->m_progress.erase(u++);
			} else {
                ++u;
            }
		}
		d->mutex->unlock();
	}
	return FALSE;
}

ReceiveFile::ReceiveFile(gloox::Bytestream *stream, std::string filename, int size, MyMutex *mutex, FileTransferManager *manager) {
    m_stream = stream;
    m_size = size;
    m_filename = filename;
//     m_stream->registerSOCKS5BytestreamDataHandler(this);
    m_finished = false;
	m_mutex = mutex;
	m_parent = manager;
    if(!m_stream->connect()) {
        Log().Get(m_filename) << "connection can't be established!";
        return;
    }
    run();
}

ReceiveFile::~ReceiveFile() {

}

void ReceiveFile::exec() {
	Log().Get(m_filename) << "starting receiveFile thread";
// 	m_stream->handleConnect(m_stream->connectionImpl());
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
        Log().Get(m_filename) << "can't create this file!";
        return;
    }
    Log().Get(m_filename) << "begin receiving this file";
    while (!m_finished) {
        m_stream->recv();
    }
    m_file.close();
	m_mutex->lock();
	Log().Get(m_filename) << "scheduling file resend to legacy network";
	m_parent->m_progress[m_stream->sid()].state=1;
	g_timeout_add(1000,&transferFinished,m_parent);
	m_mutex->unlock();
//     m_finished = true;
}

void ReceiveFile::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
    m_file.write(data.c_str(), data.size());
}

void ReceiveFile::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {
	Log().Get(m_filename) << "STREAM ERROR!";
// 	Log().Get(m_filename) << stanza->xml();
}

void ReceiveFile::handleBytestreamOpen(gloox::Bytestream *s5b) {
    Log().Get(m_filename) << "stream opened...";
}

void ReceiveFile::handleBytestreamClose(gloox::Bytestream *s5b) {
    if (m_finished){
		Log().Get(m_filename) << "Transfer finished and we're already finished => deleting receiveFile thread";
// 		delete this;
	}
	else{
		Log().Get(m_filename) << "Transfer finished";
		m_finished = true;
	}
}
