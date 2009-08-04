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
	while (!m_finished) {
		m_stream->recv();
	}
}

void ReceiveFileStraight::handleBytestreamData(gloox::Bytestream *s5b, const std::string &data) {
	getMutex()->lock();
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



static size_t ui_read_fnc(guchar **buffer, size_t size, PurpleXfer *xfer) {
	Log().Get("REPEATER") << "ui_read";
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
		(*buffer) = (guchar*) g_strdup(data.c_str());
		size_t s = repeater->getBuffer().size();
		Log().Get("REPEATER") << "GOT BUFFER, SIZE=" << s;
		repeater->getResender()->getMutex()->unlock();
		return size;
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
}

void FiletransferRepeater::registerXfer(PurpleXfer *xfer) {
	m_xfer = xfer;
	
	purple_xfer_set_ui_read_fnc(m_xfer, ui_read_fnc);
// 	purple_xfer_set_local_filename(xfer, filename);
	purple_xfer_set_size(xfer, m_size);
	m_xfer->ui_data = this;
}

void FiletransferRepeater::fileSendStart() {
	Log().Get("ReceiveFileStraight") << "fileSendStart!" << m_from.full() << " " << m_to.full();
	m_main->ft->acceptFT(m_to, m_sid, m_type, m_from.resource().empty() ? std::string(m_from.bare() + "/bot") : m_from);
}

void FiletransferRepeater::handleFTReceiveBytestream(Bytestream *bs) {
	Log().Get("ReceiveFileStraight") << "new!";
	m_resender = new ReceiveFileStraight(bs, 0, this);
}

void FiletransferRepeater::gotData(const std::string &data) {
	m_buffer.append(data);
	Log().Get("ReceiveFileStraight") << "Got DATA! " << m_wantsData;
	if (m_wantsData) {
		m_wantsData = false;
		purple_timeout_add(0,&ui_got_data,m_xfer);
	}
}


