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

#include "sendfile.h"

SendFile::SendFile(gloox::Bytestream *stream, std::string filename, int size, MyMutex *mutex, FileTransferManager *manager) {
    std::cout << "SendFile::SendFile" << " Constructor.\n";
    m_stream = stream;
    m_filename = filename;
    m_size = size;
	m_stream->registerBytestreamDataHandler (this);
	m_mutex = mutex;
	m_parent = manager;
	if (m_stream->connect())
		std::cout << "stream connected\n";
    run();
}

SendFile::~SendFile() {
//     Log::i().log(LogStageDebug, "SendFile::~SendFile", "Destructor.");
}

void SendFile::exec() {
    std::cout << "SendFile::exec" << " Start sending file " << m_filename << ".\n";
    m_file.open(m_filename.c_str(), std::ios_base::in | std::ios_base::binary );
    if (!m_file) {
        std::cout << "SendFile::exec Couldn't open file " << m_filename << "!\n";
        return;
    }
    std::cout << "SendFile::exec Starting transfer.\n";
    char input[200024];
	int ret;
    while (!m_file.eof()) {
		if (m_stream->isOpen()){
			std::cout << "sending...\n";
			m_file.read(input, 200024);
			std::string t(input, m_file.gcount());
			ret=m_stream->send(t);
			std::cout << ret << "\n";
			if(ret<1){
				std::cout << "error in sending or sending probably finished\n";
				break;
			};
		}
		m_stream->recv(200);
    }
    m_file.close();
    delete this;
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
