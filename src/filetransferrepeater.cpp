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
	FiletransferRepeater *repeater = (FiletransferRepeater *) data;
	repeater->ui_ready_callback();
	return FALSE;
}

static gboolean try_to_delete_me(gpointer data){
	FiletransferRepeater *repeater = (FiletransferRepeater *) data;
	repeater->tryToDeleteMe();
	return FALSE;
}

static gpointer sendFileCallback(gpointer data) {
	SendFile *resender = (SendFile *) data;
	while (true) {
		if (!resender->send())
			break;
	}
	resender->stopped();
	return NULL;
}

SendFile::SendFile(Bytestream *stream, int size, const std::string &filename, AbstractUser *user, FiletransferRepeater *manager) {
    m_stream = stream;
    m_size = size;
	m_filename = filename;
	m_user = user;
	m_stream->registerBytestreamDataHandler (this);
	m_parent = manager;
	if (!m_stream->connect()) {
		// TODO:
	}
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
        // TODO:
    }
    run(&sendFileCallback, this);
}

SendFile::~SendFile() {

}

bool SendFile::send() {
	if (m_stream->isOpen()) {
		lockMutex();
		std::string data;
		int size = m_parent->getDataToSend(data);
		
		if (size != 0) {
			if (m_file.is_open()) {
				m_file.write(data.c_str(), size);
			}
		}
		unlockMutex();
		m_stream->recv(200);
	}
	if (shouldStop() || !m_stream->isOpen()) {
		m_file.close();
		return false;
	}
	return true;
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
	resender->stopped();
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
	run(&sendFileStraightCallback, this);
}

SendFileStraight::~SendFileStraight() {

}

void SendFileStraight::exec() { }

bool SendFileStraight::send() {
//     char input[200024];
	if (m_stream->isOpen()){
		std::cout << "BEFORE MUTEX LOCK\n";
		lockMutex();
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
		unlockMutex();
	}
	else {
		std::cout << "stream not open!\n";
		g_usleep(G_USEC_PER_SEC);
	}
	if (m_stream->recv(2000) != ConnNoError) {
		g_timeout_add(0, &try_to_delete_me, m_parent);
		return false;
	}
	if (shouldStop()) {
		Log("SendFileStraight", "stopping thread");
		return false;
	}
	
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

static gpointer receiveFileCallback(gpointer data) {
	ReceiveFile *resender = (ReceiveFile *) data;
	while (true) {
		if (!resender->receive())
			break;
	}
	resender->stopped();
	return NULL;
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
		// TODO:
    }
    m_file.open(m_filename.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!m_file) {
        // TODO;
    }
    run(receiveFileCallback, this);
}

ReceiveFile::~ReceiveFile() {

}

void ReceiveFile::dispose() {
	Transport::instance()->disposeBytestream(m_stream);
}

bool ReceiveFile::receive() {
	if (m_stream->recv() != ConnNoError || shouldStop()) {
		m_file.close();
		Log("ReceiveFile", "transferFinished");
		g_timeout_add(1000,&transferFinished,this);
		return false;
	}
	return true;
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
	stop();
}

static gpointer receiveFunc(gpointer data) {
	ReceiveFileStraight *repeater = (ReceiveFileStraight *) data;
	while (true) {
		if (!repeater->receive())
			break;
	}
	repeater->stopped();
	return NULL;
}

ReceiveFileStraight::ReceiveFileStraight(gloox::Bytestream *stream, FiletransferRepeater *manager) {
	m_stream = stream;
	m_finished = false;
	m_parent = manager;
	m_stream->registerBytestreamDataHandler(this);
	if (!m_stream->connect()) {
		Log("ReceiveFileStraight", "connection can't be established!");
		return;
	}
	run(&receiveFunc, this);
}

ReceiveFileStraight::~ReceiveFileStraight() {
}

bool ReceiveFileStraight::receive() {
	if (m_stream->recv() != ConnNoError) {
		Log("ReceiveFileStraight", "socket closed => stopping thread");
		g_timeout_add(0, &try_to_delete_me, m_parent);
		return false;
	}
	if (shouldStop()) {
		Log("ReceiveFileStraight", "stopping thread");
		return false;
	}
	return true;
}

void ReceiveFileStraight::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	lockMutex();
	if (m_parent->handleGlooxData(data))
		wait();
	unlockMutex();
}

void ReceiveFileStraight::handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq) {
}

void ReceiveFileStraight::handleBytestreamOpen(gloox::Bytestream *s5b) {
}

void ReceiveFileStraight::handleBytestreamClose(gloox::Bytestream *s5b) {
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
	m_resender = NULL;
	m_send = false;
	m_readyCalled = false;
	m_readyTimer = 0;
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
	m_readyCalled = false;
	m_readyTimer = 0;
}

FiletransferRepeater::~FiletransferRepeater() {
	Log("xferdestroyed", "in ftrepeater");
	if (m_xfer) {
		if (m_resender) {
			Log("xferdestroyed", m_resender);
			if (m_resender->isRunning()) {
				Log("xferdestroyed", "resender is running, trying to stop it");
				m_resender->stop();
				m_resender->lockMutex();
				m_resender->wakeUp();
				m_resender->unlockMutex();
				m_resender->join();
				Log("xferdestroyed", "resender stopped.");
			}
			Log("xferdestroyed", m_resender);
			delete m_resender;
			m_resender = NULL;
		}
		if (m_readyTimer != 0)
			purple_timeout_remove(m_readyTimer);
		m_xfer->ui_data = NULL;
		purple_xfer_unref(m_xfer);
		m_xfer = NULL;
	}
}

void FiletransferRepeater::registerXfer(PurpleXfer *xfer) {
	m_xfer = xfer;
	purple_xfer_ref(m_xfer);

	if (m_size != -1)
		purple_xfer_set_size(xfer, m_size);

	m_xfer->ui_data = this;
}

void FiletransferRepeater::fileSendStart() {
	Log("FiletransferRepeater::fileSendStart", "accepting FT, data:" << m_from.full() << " " << m_to.full());
	Transport::instance()->acceptFT(m_to, m_sid, m_type, m_from.resource().empty() ? std::string(m_from.bare() + "/bot") : m_from);
}

void FiletransferRepeater::fileRecvStart() {
	Log("FiletransferRepeater::fileRecvStart", "accepting FT, data:" << m_from.full() << " " << m_to.full());
	// We have to say to libpurple that we are ready to receive first data...
	if (m_resender && m_xfer)
		g_timeout_add(0, &ui_got_data, this);
}

std::string FiletransferRepeater::requestFT() {
	std::string filename(m_xfer ? purple_xfer_get_filename(m_xfer) : "");
	m_sid = Transport::instance()->requestFT(m_to, filename, purple_xfer_get_size(m_xfer), EmptyString, EmptyString, EmptyString, EmptyString, SIProfileFT::FTTypeAll, m_from);
	return m_sid;
}

void FiletransferRepeater::handleFTReceiveBytestream(Bytestream *bs, const std::string &filename) {
	if (filename.empty())
		m_resender = new ReceiveFileStraight(bs, this);
	else {
		AbstractUser *user = Transport::instance()->userManager()->getUserByJID(bs->initiator().bare());
		m_resender = new ReceiveFile(bs, 0, filename, user, this);
	}
}

void FiletransferRepeater::handleFTSendBytestream(Bytestream *bs, const std::string &filename) {
	if (!m_xfer) {
		Log("FiletransferRepeater::handleFTSendBytestream", "no xfer");
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

bool FiletransferRepeater::handleGlooxData(const std::string &data) {
	std::cout << "FiletransferRepeater::handleGlooxData " << data.size() << "\n";
	// If buffer was empty, we have to say to libpurple that we're ready and have new data for it.
	if (m_buffer_size == 0)
		ready();

	// Append new data to buffer
	guchar *c = m_buffer + m_buffer_size;
	memcpy(c, data.c_str(), data.size());
	m_buffer_size += data.size();

	// If we have too much data, repeater will just wait for libpurple to handle it.
	bool wait = m_buffer_size > 5000;
	return wait;
}

gssize FiletransferRepeater::handleLibpurpleData(const guchar *data, gssize size) {
	std::cout << "FiletransferRepeater::handleLibpurpleData " << m_buffer_size + size << "\n";
	if (m_resender)
		m_resender->lockMutex();

	// Increase buffer size if we've too much data.
	gssize data_size = size;
	if (m_buffer_size + data_size > m_max_buffer_size) {
		m_max_buffer_size = data_size + m_buffer_size;
		m_buffer = (guchar *) g_realloc(m_buffer, m_max_buffer_size);
		std::cout << "new m_max_buffer_size is " << m_max_buffer_size << "\n";
	}

	// Append new data to buffer.
	guchar *c = m_buffer + m_buffer_size;
	memcpy(c, data, data_size);
	m_buffer_size += data_size;

	// Wake up resender thread and let it handle new data.
	m_resender->wakeUp();

	if (m_resender)
		m_resender->unlockMutex();

	return data_size;
}

void FiletransferRepeater::handleDataNotSent(const guchar *data, gssize size) {
	if (m_resender)
		m_resender->lockMutex();

	// Move data from zero to `size` (shift right)
	guchar *oldbufferdata = m_buffer + size;
	memcpy(oldbufferdata, m_buffer, m_buffer_size);
	m_buffer_size = m_buffer_size + size;

	// Copy data to buffer
	memcpy(m_buffer, data, size);

	// We've unset data again, so we're ready.
	ready();
	if (m_resender)
		m_resender->unlockMutex();
}

int FiletransferRepeater::getDataToSend(guchar **data, gssize size) {
	if (m_resender)
		m_resender->lockMutex();
	std::cout << "buffer size:" << m_buffer_size << "\n";
	int data_size = 0;

	if ((gssize) m_buffer_size > size) {
		// Store first `size` bytes into `data`.
		(*data) = (guchar *) g_malloc0(size);
		memcpy((*data), m_buffer, size);
		data_size = (int) size;

		// Replace sent data with the new one (just shift substring in m_buffer starting at `size` left to zero).
		guchar *newbufferdata = m_buffer + size;
		memcpy(m_buffer, newbufferdata, m_buffer_size - size);
		m_buffer_size = m_buffer_size - size;
	}
	else {
		// Copy whole buffer and mark it as sent.
		(*data) = (guchar *) g_malloc0(m_buffer_size);
		memcpy((*data), m_buffer, m_buffer_size);
		data_size = (int) m_buffer_size;
		m_buffer_size = 0;
	}

	m_readyCalled = false;

	// If we have almost empty buffer, wake up resender and let it receive some new data.
	if (m_buffer_size < 1000) {
		Log("REPEATER", "WakeUP");
		if (m_resender)
			m_resender->wakeUp();
		}
	else
		ready();
	
	if (m_resender)
		m_resender->unlockMutex();

	// Try to finish the transfer if we can't have new data and buffer is empty.
	if (m_resender && m_resender->isRunning() == false && m_buffer_size == 0) {
		g_timeout_add(0, &try_to_delete_me, this);
	}

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
		m_readyTimer = g_timeout_add(0, &ui_got_data, this);
	m_readyCalled = true;
}

void FiletransferRepeater::ui_ready_callback() {
	purple_xfer_ui_ready(m_xfer);
}

void FiletransferRepeater::tryToDeleteMe() {
	if (purple_xfer_get_status(m_xfer) == PURPLE_XFER_STATUS_DONE && m_buffer_size == 0) {
		Log("xfer-tryToDeleteMe", "xfer is done, buffer_size = 0 => finishing it and removing repeater");
		delete this;
	}
	else {
		Log("xfer-tryToDeleteMe", "can't delete, because status is " << (int) purple_xfer_get_status(m_xfer));
	}
}
