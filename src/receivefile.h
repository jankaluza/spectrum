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

#ifndef RECEIVEFILE_H
#define RECEIVEFILE_H
#include "thread.h"

#include <gloox/bytestreamdatahandler.h>
#include <gloox/bytestream.h>
class FileTransferManager;
class GlooxMessageHandler;
#include <iostream>
#include <fstream>
#include <string>


struct progress;

class ReceiveFile : public gloox::BytestreamDataHandler,
                    public Thread {
public:
	gloox::Bytestream *m_stream;
    std::string m_filename;
    int m_size;
    bool m_finished;

	MyMutex *m_mutex;
	FileTransferManager *m_parent;

    std::ofstream m_file;

    ReceiveFile(gloox::Bytestream *stream, const std::string &filename, int size, MyMutex *mutex, FileTransferManager *manager);

    ~ReceiveFile();

    void exec();

    void handleBytestreamData(gloox::Bytestream *s5b, const std::string &data);

    void handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq);

    void handleBytestreamOpen(gloox::Bytestream *s5b);

    void handleBytestreamClose(gloox::Bytestream *s5b);
};

#endif
