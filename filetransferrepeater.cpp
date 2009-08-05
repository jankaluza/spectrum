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
#include "main.h"

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
    char input[200024];
	int ret;
	bool empty;
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
				ret = m_stream->send(t);
				std::cout << ret << "\n";
				if(ret<1){
					std::cout << "error in sending or sending probably finished\n";
					break;
				};
			}
		}
		m_stream->recv(200);
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
	getMutex()->unlock();
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

static gboolean ui_got_data(gpointer data){
	PurpleXfer *xfer = (PurpleXfer*) data;
	purple_xfer_ui_got_data(xfer);
	return FALSE;
}

static size_t ui_write_fnc(const guchar *buffer, size_t size, PurpleXfer *xfer) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	std::string d((char *)buffer, size);
	if (repeater->getResender())
		repeater->getResender()->getMutex()->lock();	
	repeater->gotData(d);
	if (repeater->getResender())
		repeater->getResender()->getMutex()->unlock();
	return size;
}

static size_t ui_read_fnc(guchar **buffer, size_t size, PurpleXfer *xfer) {
// 	Log().Get("REPEATER") << "ui_read";
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	if (!repeater->getResender()) {
		repeater->wantsData();
		(*buffer) = (guchar*) g_strdup("");
		return 0;
	}
	repeater->getResender()->getMutex()->lock();
	if (repeater->getBuffer().empty()) {
		Log().Get("REPEATER") << "buffer is empty, setting wantsData = true";
		repeater->wantsData();
		(*buffer) = (guchar*) g_strdup("");
		repeater->getResender()->getMutex()->unlock();
		return 0;
	}
	else {
		std::string data;
		if (repeater->getBuffer().size() > size) {
			data = repeater->getBuffer().substr(0, size);
			repeater->getBuffer().erase(0, size);
		}
		else {
			data = repeater->getBuffer();
			repeater->getBuffer().erase();
		}
// 		(*buffer) = (guchar*) g_strndup(data.c_str(), data.size());
		memcpy((*buffer), data.c_str(), data.size());
		size_t s = repeater->getBuffer().size();
		Log().Get("REPEATER") << "GOT BUFFER, BUFFER SIZE=" << s;
		repeater->getResender()->getMutex()->unlock();
		return data.size();
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
	m_file.open("dump.bin", std::ios_base::out | std::ios_base::binary );
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
}

void FiletransferRepeater::registerXfer(PurpleXfer *xfer) {
	m_xfer = xfer;
	
	purple_xfer_set_ui_read_fnc(m_xfer, ui_read_fnc);
	purple_xfer_set_ui_write_fnc(m_xfer, ui_write_fnc);
// 	purple_xfer_set_local_filename(xfer, filename);
	if (m_size != -1)
		purple_xfer_set_size(xfer, m_size);
	m_xfer->ui_data = this;
}

void FiletransferRepeater::fileSendStart() {
	Log().Get("ReceiveFileStraight") << "fileSendStart!" << m_from.full() << " " << m_to.full();
	m_main->ft->acceptFT(m_to, m_sid, m_type, m_from.resource().empty() ? std::string(m_from.bare() + "/bot") : m_from);
}

void FiletransferRepeater::fileRecvStart() {
	Log().Get("SendFileStraight") << "fileRecvStart!" << m_from.full() << " " << m_to.full();
}

void FiletransferRepeater::requestFT() {
// 	purple_xfer_request_accepted(xfer, std::string(filename).c_str());
	std::string filename(purple_xfer_get_filename(m_xfer));
	m_sid = m_main->ft->requestFT(m_to, filename, purple_xfer_get_size(m_xfer), EmptyString, EmptyString, EmptyString, EmptyString, SIProfileFT::FTTypeAll, m_from);
}

void FiletransferRepeater::handleFTReceiveBytestream(Bytestream *bs) {
	Log().Get("ReceiveFileStraight") << "new!";
	m_resender = new ReceiveFileStraight(bs, 0, this);
}

void FiletransferRepeater::handleFTSendBytestream(Bytestream *bs) {
	Log().Get("SendFileStraight") << "new!";
	purple_xfer_request_accepted(m_xfer, NULL);
	m_resender = new SendFileStraight(bs, 0, this);
}

void FiletransferRepeater::gotData(const std::string &data) {
	m_buffer.append(std::string(data));
// 	Log().Get("ReceiveFileStraight") << "Got DATA! " << m_wantsData;
	if (m_wantsData) {
		m_wantsData = false;
		purple_timeout_add(0,&ui_got_data,m_xfer);
	}
}


