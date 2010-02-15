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
#include "abstractuser.h"
#include "transport.h"
#include "spectrummessagehandler.h"

static gboolean ui_got_data(gpointer data){
	PurpleXfer *xfer = (PurpleXfer*) data;
	purple_xfer_ui_ready(xfer);
	std::cout << "READY!!\n";
	return FALSE;
}

SendFile::SendFile(Bytestream *stream, int size, const std::string &filename, AbstractUser *user, FiletransferRepeater *manager) {
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
	bool empty;
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
//         Log().Get(m_filename) << "can't create this file!";
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
			g_mutex_lock(getMutex());
			std::string t;
			if (m_parent->getBuffer().empty())
				empty = true;
			else {
				empty = false;
				t = std::string(m_parent->getBuffer());
				m_parent->getBuffer().erase();
			}
			g_mutex_unlock(getMutex());
			if (!empty) {
				m_file.write(t.c_str(), t.size());
			}
		}
		m_stream->recv(200);
    }
	m_file.close();
	dispose();
    delete this;
}

void SendFile::dispose() {
	Transport::instance()->disposeBytestream(m_stream);
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

static gpointer sendFileStraightCallback(gpointer data) {
	SendFileStraight *resender = (SendFileStraight *) data;
	while (true) {
		if (!resender->send())
			break;
	}
	return NULL;
}

SendFileStraight::SendFileStraight(Bytestream *stream, int size, FiletransferRepeater *manager) {
    std::cout << "SendFileStraight::SendFileStraight" << " Constructor.\n";
    m_stream = stream;
    m_size = size;
	m_stream->registerBytestreamDataHandler (this);
	m_parent = manager;
	if (m_stream->connect())
		std::cout << "stream connected\n";
    g_thread_create(&sendFileStraightCallback, this, false, NULL);
}

SendFileStraight::~SendFileStraight() {

}

void SendFileStraight::exec() { }

bool SendFileStraight::send() {
//     char input[200024];
	if (m_stream->isOpen()){
		std::cout << "BEFORE MUTEX LOCK\n";
		g_mutex_lock(getMutex());
		std::cout << "TRYING TO SEND\n";
		std::string data;
		int size = m_parent->getDataToSend(data);
		
		if (size != 0) {
			int ret = m_stream->send(data);
			if (ret < 1) {
				std::cout << "error in sending or sending probably finished\n";
				return false;
			};
		}
		else
			wait();
		g_mutex_unlock(getMutex());
	}
	else {
		std::cout << "stream not open!\n";
	}
	if (m_stream->recv(2000) != ConnNoError)
		return false;
	return true;
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
	AbstractUser *user = receive->user();
	std::string filename(receive->filename());
	Log(user->jid(), "trying to send file "<< filename);
	if (user->account()){
		if (user->isConnected()){
			Log(user->jid(), "sending download message");
			std::vector <std::string> dirs = split(filename, '/');
			std::string url = Transport::instance()->getConfiguration().filetransferWeb;
			std::string basename = dirs.back();
			dirs.pop_back();
			url += dirs.back() + "/";
			url += basename;
			Message s(Message::Chat, receive->target(), "User sent you file '"+basename+"'. You can download it from " + url +" .");
			s.setFrom(user->jid());
			// TODO: rewrite me to not use GlooxMessageHandler
#ifndef TESTS
			GlooxMessageHandler::instance()->handleMessage(s, NULL);
#endif
			
		}
	}
	receive->dispose();
	return FALSE;
}

ReceiveFile::ReceiveFile(gloox::Bytestream *stream, int size, const std::string &filename, AbstractUser *user, FiletransferRepeater *manager) {
    m_stream = stream;
    m_size = size;
	m_filename = filename;
	m_user = user;
	m_stream->registerBytestreamDataHandler (this);
	m_target = stream->target().bare();
    m_finished = false;
	m_parent = manager;
    if(!m_stream->connect()) {
//         Log().Get("ReceiveFile") << "connection can't be established!";
        return;
    }
    run();
}

ReceiveFile::~ReceiveFile() {

}

void ReceiveFile::dispose() {
	Transport::instance()->disposeBytestream(m_stream);
}

void ReceiveFile::exec() {
// 	Log().Get("ReceiveFile") << "starting receiveFile thread";
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
//         Log().Get(m_filename) << "can't create this file!";
        return;
    }
	while (!m_finished) {
		Log("ReceiveFile", "recv");
		m_stream->recv();
	}
	m_file.close();
	Log("ReceiveFile", "transferFinished");
	g_timeout_add(1000,&transferFinished,this);
}

void ReceiveFile::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	m_file.write(data.c_str(), data.size());
}

void ReceiveFile::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {
// 	Log().Get("ReceiveFile") << "STREAM ERROR!";
// 	Log().Get("ReceiveFile") << stanza->xml();
}

void ReceiveFile::handleBytestreamOpen(gloox::Bytestream *s5b) {
// 	Log().Get("ReceiveFile") << "stream opened...";
}

void ReceiveFile::handleBytestreamClose(gloox::Bytestream *s5b) {
    if (m_finished){
// 		Log().Get("ReceiveFile") << "Transfer finished and we're already finished => deleting receiveFile thread";
		delete this;
	}
	else{
// 		Log().Get("ReceiveFile") << "Transfer finished";
		m_finished = true;
	}
}

static gpointer receiveFunc(gpointer data) {
	gloox::Bytestream *stream = (gloox::Bytestream *) data;
	while (true) {
		if (stream->recv() != ConnNoError)
			break;
	}
	return NULL;
}

ReceiveFileStraight::ReceiveFileStraight(gloox::Bytestream *stream, int size, FiletransferRepeater *manager) {
    m_stream = stream;
    m_size = size;
	m_stream->registerBytestreamDataHandler (this);
    m_finished = false;
	m_parent = manager;
    if(!m_stream->connect()) {
//         Log().Get("ReceiveFileStraight") << "connection can't be established!";
        return;
    }
//     run();
	g_thread_create(&receiveFunc, m_stream, false, NULL);
	
}

ReceiveFileStraight::~ReceiveFileStraight() {

}

void ReceiveFileStraight::exec() {
// 	Log().Get("ReceiveFileStraight") << "starting receiveFile thread";
// 	m_stream->handleConnect(m_stream->connectionImpl());
// 	Log().Get("ReceiveFileStraight") << "begin receiving this file";
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
	bool w;
	g_mutex_lock(getMutex());
// 	m_file.write(data.c_str(), data.size());
	w = m_parent->gotData(data);
	if (w) {
		wait();
	}
	g_mutex_unlock(getMutex());
}

void ReceiveFileStraight::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {
// 	Log().Get("ReceiveFileStraight") << "STREAM ERROR!";
// 	Log().Get("ReceiveFileStraight") << stanza->xml();
}

void ReceiveFileStraight::handleBytestreamOpen(gloox::Bytestream *s5b) {
// 	Log().Get("ReceiveFileStraight") << "stream opened...";
}

void ReceiveFileStraight::handleBytestreamClose(gloox::Bytestream *s5b) {
    if (m_finished){
// 		Log().Get("ReceiveFileStraight") << "Transfer finished and we're already finished => deleting receiveFile thread";
		delete this;
	}
	else{
// 		Log().Get("ReceiveFileStraight") << "Transfer finished";
		m_finished = true;
	}
}

FiletransferRepeater::FiletransferRepeater(const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size) {
	m_size = size;
	m_to = to;
	m_sid = sid;
	m_type = type;
	m_from = from;
	m_max_buffer_size = 65535;
	m_buffer = (guchar *) g_malloc0(m_max_buffer_size);
	m_buffer_size = 0;
	m_wantsData = false;
	m_resender = NULL;
	m_send = false;
	m_readyCalled = false;
}

FiletransferRepeater::FiletransferRepeater(const JID& from, const JID& to) {
	m_to = to;
	m_from = from;
	m_max_buffer_size = 65535;
	m_buffer = (guchar *) g_malloc0(m_max_buffer_size);
	m_buffer_size = 0;
	m_type = SIProfileFT::FTTypeS5B;
	m_resender = NULL;
	m_size = -1;
	m_send = true;
	m_wantsData = false;
	m_readyCalled = false;
}

void FiletransferRepeater::registerXfer(PurpleXfer *xfer) {
	m_xfer = xfer;

// 	purple_xfer_set_local_filename(xfer, filename);
	if (m_size != -1)
		purple_xfer_set_size(xfer, m_size);
	m_xfer->ui_data = this;
}

void FiletransferRepeater::fileSendStart() {
	Log("ReceiveFileStraight", "fileSendStart!" << m_from.full() << " " << m_to.full());
	Log("ReceiveFileStraight", m_sid);
	Log("ReceiveFileStraight", (int)m_type);
	
	Transport::instance()->acceptFT(m_to, m_sid, m_type, m_from.resource().empty() ? std::string(m_from.bare() + "/bot") : m_from);
}

void FiletransferRepeater::fileRecvStart() {
	Log("SendFileStraight", "fileRecvStart!" << m_from.full() << " " << m_to.full());
	if (m_resender && m_xfer)
		g_timeout_add(0, &ui_got_data, m_xfer);
}

std::string FiletransferRepeater::requestFT() {
	std::string filename(m_xfer ? purple_xfer_get_filename(m_xfer) : "");
	m_sid = Transport::instance()->requestFT(m_to, filename, purple_xfer_get_size(m_xfer), EmptyString, EmptyString, EmptyString, EmptyString, SIProfileFT::FTTypeAll, m_from);
	return m_sid;
}

void FiletransferRepeater::handleFTReceiveBytestream(Bytestream *bs, const std::string &filename) {
	Log("ReceiveFileStraight", "new!");
	if (filename.empty())
		m_resender = new ReceiveFileStraight(bs, 0, this);
	else {
		AbstractUser *user = Transport::instance()->userManager()->getUserByJID(bs->initiator().bare());
		m_resender = new ReceiveFile(bs, 0, filename, user, this);
	}
}

void FiletransferRepeater::handleFTSendBytestream(Bytestream *bs, const std::string &filename) {
	Log("SendFileStraight", "new!");
	if (!m_xfer) {
		Log("SendFileStraight", "no xfer");
		return;
	}
	purple_xfer_request_accepted(m_xfer, NULL);
	if (filename.empty())
		m_resender = new SendFileStraight(bs, 0, this);
	else {
		AbstractUser *user = Transport::instance()->userManager()->getUserByJID(bs->initiator().bare());
		m_resender = new SendFile(bs, 0, filename, user, this);
	}
}

bool FiletransferRepeater::gotData(const std::string &data) {
	std::cout << "FiletransferRepeater::gotData " << data.size() << "\n";
	if (m_buffer_size == 0)
		ready();
	guchar *c = m_buffer + m_buffer_size;
	memcpy(c, data.c_str(), data.size());
	m_buffer_size += data.size();

// 	for (unsigned int i = 0; i < data.size(); i++) {
// 		m_buffer->append("a");
// 	}
	
	if (m_send) {
		std::cout << "WakeUP\n";
		m_resender->wakeUp();
	}
	bool wait = m_buffer_size > 5000;
// 	if (wait)
// 		g_timeout_add(7,&getData,this);
	return wait;
}

gssize FiletransferRepeater::handleLibpurpleData(const guchar *data, gssize size) {
	std::cout << "FiletransferRepeater::handleLibpurpleData " << m_buffer_size + size << "\n";
	if (m_resender)
		g_mutex_lock(m_resender->getMutex());
	gssize data_size = size;
	if (m_buffer_size + data_size > m_max_buffer_size) {
		m_max_buffer_size = data_size + m_buffer_size;
		m_buffer = (guchar *) g_realloc(m_buffer, m_max_buffer_size);
		std::cout << "new m_max_buffer_size is " << m_max_buffer_size << "\n";
	}

	guchar *c = m_buffer + m_buffer_size;
	memcpy(c, data, data_size);
	m_buffer_size += data_size;

	m_resender->wakeUp();

	if (m_resender)
		g_mutex_unlock(m_resender->getMutex());
	return data_size;
}

void FiletransferRepeater::handleDataNotSent(const guchar *data, gssize size) {
	if (m_resender)
		g_mutex_lock(m_resender->getMutex());

	// Move data from zero to `size` (shift right)
	guchar *oldbufferdata = m_buffer + size;
	memcpy(oldbufferdata, m_buffer, m_buffer_size);
	m_buffer_size = m_buffer_size + size;

	// Copy data to buffer
	memcpy(m_buffer, data, size);
	
	ready();
	if (m_resender)
		g_mutex_unlock(m_resender->getMutex());
}

int FiletransferRepeater::getDataToSend(guchar **data, gssize size) {
	if (m_resender)
		g_mutex_lock(m_resender->getMutex());
	std::cout << "buffer size:" << m_buffer_size << "\n";
	int data_size = 0;

	if ((gssize) m_buffer_size > size) {
		// Store `size` bytes into `data`.
		(*data) = (guchar *) g_malloc0(size);
		memcpy((*data), m_buffer, size);
		data_size = (int) size;

		// Replace sent data with the new one (just shift substring in m_buffer starting at `size` left to zero).
		guchar *newbufferdata = m_buffer + size;
		memcpy(m_buffer, newbufferdata, m_buffer_size - size);
		m_buffer_size = m_buffer_size - size;
	}
	else {
		(*data) = (guchar *) g_malloc0(m_buffer_size);
		memcpy((*data), m_buffer, m_buffer_size);
		data_size = (int) m_buffer_size;
		m_buffer_size = 0;
	}

	bool wakeUp = m_buffer_size < 1000;
	m_readyCalled = false;
	
	if (wakeUp) {
		Log("REPEATER", "WakeUP");
		if (m_resender)
			m_resender->wakeUp();
		}
	else
		ready();
	
	if (m_resender)
		g_mutex_unlock(m_resender->getMutex());

	return data_size;
}

int FiletransferRepeater::getDataToSend(std::string &data) {
	int size = 0;
	if (m_buffer_size != 0) {
		data.assign((char *) m_buffer, m_buffer_size);
		size = m_buffer_size;
		m_buffer_size = 0;
	}
	m_readyCalled = false;
	ready();
	return size;
}

void FiletransferRepeater::ready() {
	if (!m_readyCalled && m_xfer)
		g_timeout_add(0, &ui_got_data, m_xfer);
	m_readyCalled = true;
}

void FiletransferRepeater::xferDestroyed() {
	m_xfer = NULL;
	if (!m_resender)
		delete this;
}


