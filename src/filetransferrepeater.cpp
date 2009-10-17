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

#include "filetransferrepeater.h"
#include "filetransfermanager.h"
#include "main.h"
#include "usermanager.h"
#include "log.h"
#include "sql.h"

static gboolean ui_got_data(gpointer data){
	PurpleXfer *xfer = (PurpleXfer*) data;
	purple_xfer_ui_ready(xfer);
	return FALSE;
}


SendFile::SendFile(Bytestream *stream, int size, const std::string &filename, User *user, FiletransferRepeater *manager) {
    std::cout << "SendFile::SendFile" << " Constructor.\n";
    m_stream = stream;
    m_size = size;
	m_filename = filename;
	m_user = user;
	m_stream->registerBytestreamDataHandler (this);
	m_parent = manager;
	if (m_stream->connect())
		std::cout << "stream connected\n";
    run();
}

SendFile::~SendFile() {

}

void SendFile::exec() {
    std::cout << "SendFile::exec Starting transfer.\n";
//     char input[200024];
	int ret;
	bool empty;
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
        Log().Get(m_filename) << "can't create this file!";
        return;
    }
    while (true) {
		if (m_stream->isOpen()){
// 			std::cout << "sending...\n";
			std::string data;
// 			if (getBuffer().size() > size) {
// 				data = repeater->getBuffer().substr(0, size);
// 				repeater->getBuffer().erase(0, size);
// 			}
// 			else {
// 			getBuffer();
// 			getBuffer().erase();
// 			}
// 			m_file.read(input, 200024);
			getMutex()->lock();
			std::string t;
			if (m_parent->getBuffer().empty())
				empty = true;
			else {
				empty = false;
				t = std::string(m_parent->getBuffer());
				m_parent->getBuffer().erase();
			}
			getMutex()->unlock();
			if (!empty) {
				m_file.write(t.c_str(), t.size());
				if(ret<1){
					std::cout << "error in sending or sending probably finished\n";
					break;
				};
			}
		}
		m_stream->recv(200);
    }
	m_file.close();
	dispose();
    delete this;
}

void SendFile::dispose() {
	m_parent->parent()->ft->dispose(m_stream);
}

void SendFile::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	std::cout << "socks stream data\n";
}

void SendFile::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {

}

void SendFile::handleBytestreamOpen(gloox::Bytestream *s5b) {
	std::cout << "socks stream open\n";
}

void SendFile::handleBytestreamClose(gloox::Bytestream *s5b) {
	std::cout << "socks stream error\n";
}

SendFileStraight::SendFileStraight(Bytestream *stream, int size, FiletransferRepeater *manager) {
    std::cout << "SendFileStraight::SendFileStraight" << " Constructor.\n";
    m_stream = stream;
    m_size = size;
	m_stream->registerBytestreamDataHandler (this);
	m_parent = manager;
	if (m_stream->connect())
		std::cout << "stream connected\n";
    run();
}

SendFileStraight::~SendFileStraight() {

}

void SendFileStraight::exec() {
    std::cout << "SendFileStraight::exec Starting transfer.\n";
//     char input[200024];
	int ret;
	bool empty;
    while (true) {
		if (m_stream->isOpen()){
			std::string data;
			getMutex()->lock();
			std::string t;
			if (m_parent->getBuffer().empty()) {
				empty = true;
			}
			else {
				empty = false;
				t = std::string(m_parent->getBuffer());
				m_parent->getBuffer().erase();
			}
			m_parent->ready();
			getMutex()->unlock();
			if (!empty) {
				ret = m_stream->send(t);
				if(ret<1){
					std::cout << "error in sending or sending probably finished\n";
					break;
				};
			}
			else
				wait();
		}
		else {
			std::cout << "stream not open!\n";
		}
		if (m_stream->recv(2000) != ConnNoError)
			break;
    }
//     delete this;
}

void SendFileStraight::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	std::cout << "socks stream data\n";
}

void SendFileStraight::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {

}

void SendFileStraight::handleBytestreamOpen(gloox::Bytestream *s5b) {
	std::cout << "socks stream open\n";
}

void SendFileStraight::handleBytestreamClose(gloox::Bytestream *s5b) {
	std::cout << "socks stream error\n";
}

static gboolean transferFinished(gpointer data) {
	ReceiveFile *receive = (ReceiveFile *) data;
	User *user = receive->user();
	std::string filename(receive->filename());
	Log().Get(user->jid()) << "trying to send file "<< filename;
	if (user->account()){
		if (user->isConnected()){
			Log().Get(user->jid()) << "sending download message";
			std::string basename(g_path_get_basename(filename.c_str()));
			if (user->isVIP()) {
				user->p->sql()->addDownload(basename,"1");
			}
			else {
				user->p->sql()->addDownload(basename,"0");
			}
			Message s(Message::Chat, receive->target(), "Uzivatel Vam poslal soubor '"+basename+"'. Muzete jej stahnout z adresy http://soumar.jabbim.cz/icq/" + basename +" .");
			s.setFrom(user->jid());
			user->receivedMessage(s);
		}
	}
	receive->dispose();
	return FALSE;
}

ReceiveFile::ReceiveFile(gloox::Bytestream *stream, int size, const std::string &filename, User *user, FiletransferRepeater *manager) {
    m_stream = stream;
    m_size = size;
	m_filename = filename;
	m_user = user;
	m_stream->registerBytestreamDataHandler (this);
	m_target = stream->target().bare();
    m_finished = false;
	m_parent = manager;
    if(!m_stream->connect()) {
        Log().Get("ReceiveFile") << "connection can't be established!";
        return;
    }
    run();
}

ReceiveFile::~ReceiveFile() {

}

void ReceiveFile::dispose() {
	m_parent->parent()->ft->dispose(m_stream);
}

void ReceiveFile::exec() {
	Log().Get("ReceiveFile") << "starting receiveFile thread";
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
        Log().Get(m_filename) << "can't create this file!";
        return;
    }
	while (!m_finished) {
		m_stream->recv();
	}
	m_file.close();
	g_timeout_add(1000,&transferFinished,this);
}

void ReceiveFile::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	m_file.write(data.c_str(), data.size());
}

void ReceiveFile::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {
	Log().Get("ReceiveFile") << "STREAM ERROR!";
// 	Log().Get("ReceiveFile") << stanza->xml();
}

void ReceiveFile::handleBytestreamOpen(gloox::Bytestream *s5b) {
	Log().Get("ReceiveFile") << "stream opened...";
}

void ReceiveFile::handleBytestreamClose(gloox::Bytestream *s5b) {
    if (m_finished){
		Log().Get("ReceiveFile") << "Transfer finished and we're already finished => deleting receiveFile thread";
		delete this;
	}
	else{
		Log().Get("ReceiveFile") << "Transfer finished";
		m_finished = true;
	}
}


ReceiveFileStraight::ReceiveFileStraight(gloox::Bytestream *stream, int size, FiletransferRepeater *manager) {
    m_stream = stream;
    m_size = size;
	m_stream->registerBytestreamDataHandler (this);
    m_finished = false;
	m_parent = manager;
    if(!m_stream->connect()) {
        Log().Get("ReceiveFileStraight") << "connection can't be established!";
        return;
    }
    run();
}

ReceiveFileStraight::~ReceiveFileStraight() {

}

void ReceiveFileStraight::exec() {
	Log().Get("ReceiveFileStraight") << "starting receiveFile thread";
// 	m_stream->handleConnect(m_stream->connectionImpl());
	Log().Get("ReceiveFileStraight") << "begin receiving this file";
//     m_file.open("dump.bin", std::ios_base::out | std::ios_base::binary );
//     if (!m_file) {
//         Log().Get(m_filename) << "can't create this file!";
//         return;
//     }
	while (!m_finished) {
		m_stream->recv();
	}
// 	m_file.close();
}

void ReceiveFileStraight::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	getMutex()->lock();
// 	m_file.write(data.c_str(), data.size());
	m_parent->gotData(data);
	m_parent->ready();
	getMutex()->unlock();
	wait();
}

void ReceiveFileStraight::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {
	Log().Get("ReceiveFileStraight") << "STREAM ERROR!";
// 	Log().Get("ReceiveFileStraight") << stanza->xml();
}

void ReceiveFileStraight::handleBytestreamOpen(gloox::Bytestream *s5b) {
	Log().Get("ReceiveFileStraight") << "stream opened...";
}

void ReceiveFileStraight::handleBytestreamClose(gloox::Bytestream *s5b) {
    if (m_finished){
		Log().Get("ReceiveFileStraight") << "Transfer finished and we're already finished => deleting receiveFile thread";
		delete this;
	}
	else{
		Log().Get("ReceiveFileStraight") << "Transfer finished";
		m_finished = true;
	}
}

FiletransferRepeater::FiletransferRepeater(GlooxMessageHandler *main, const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size) {
	m_main = main;
	m_size = size;
	m_to = to;
	m_sid = sid;
	m_type = type;
	m_from = from;
	m_buffer = "";
	m_wantsData = false;
	m_resender = NULL;
	m_send = false;
}

FiletransferRepeater::FiletransferRepeater(GlooxMessageHandler *main, const JID& from, const JID& to) {
	m_main = main;
	m_to = to;
	m_from = from;
	m_buffer = "";
	m_type = SIProfileFT::FTTypeS5B;
	m_resender = NULL;
	m_size = -1;
	m_send = true;
	m_wantsData = false;
}

void FiletransferRepeater::registerXfer(PurpleXfer *xfer) {
	m_xfer = xfer;

// 	purple_xfer_set_local_filename(xfer, filename);
	if (m_size != -1)
		purple_xfer_set_size(xfer, m_size);
	m_xfer->ui_data = this;
}

void FiletransferRepeater::fileSendStart() {
	Log().Get("ReceiveFileStraight") << "fileSendStart!" << m_from.full() << " " << m_to.full();
	Log().Get("ReceiveFileStraight") << m_sid;
	Log().Get("ReceiveFileStraight") << m_type;
	m_main->ft->acceptFT(m_to, m_sid, m_type, m_from.resource().empty() ? std::string(m_from.bare() + "/bot") : m_from);
}

void FiletransferRepeater::fileRecvStart() {
	Log().Get("SendFileStraight") << "fileRecvStart!" << m_from.full() << " " << m_to.full();
}

std::string FiletransferRepeater::requestFT() {
// 	purple_xfer_request_accepted(xfer, std::string(filename).c_str());
	std::string filename(purple_xfer_get_filename(m_xfer));
	m_sid = m_main->ft->requestFT(m_to, filename, purple_xfer_get_size(m_xfer), EmptyString, EmptyString, EmptyString, EmptyString, SIProfileFT::FTTypeAll, m_from);
	m_main->ftManager->m_info[m_sid].filename = filename;
	m_main->ftManager->m_info[m_sid].size = purple_xfer_get_size(m_xfer);
	m_main->ftManager->m_info[m_sid].straight = true;
	return m_sid;
}

void FiletransferRepeater::handleFTReceiveBytestream(Bytestream *bs, const std::string &filename) {
	Log().Get("ReceiveFileStraight") << "new!";
	if (filename.empty())
		m_resender = new ReceiveFileStraight(bs, 0, this);
	else {
		User *user = m_main->userManager()->getUserByJID(bs->initiator().bare());
		m_resender = new ReceiveFile(bs, 0, filename, user, this);
	}
}

void FiletransferRepeater::handleFTSendBytestream(Bytestream *bs, const std::string &filename) {
	Log().Get("SendFileStraight") << "new!";
	purple_xfer_request_accepted(m_xfer, NULL);
	if (filename.empty())
		m_resender = new SendFileStraight(bs, 0, this);
	else {
		User *user = m_main->userManager()->getUserByJID(bs->initiator().bare());
		m_resender = new SendFile(bs, 0, filename, user, this);
	}
	purple_timeout_add_seconds(3,&ui_got_data,m_xfer);
}

void FiletransferRepeater::gotData(const std::string &data) {
	m_buffer.append(std::string(data));
	if (m_wantsData) {
		m_wantsData = false;
		purple_timeout_add(0,&ui_got_data,m_xfer);
	}
	else if (m_send)
		m_resender->wakeUp();
	else
		std::cout << "got data but don't want them yet\n";
}


