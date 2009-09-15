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

#ifndef SENDFILE_H
#define SENDFILE_H

#include "thread.h"
class FileTransferManager;
#include "filetransfermanager.h"
#include <fstream>

#include <gloox/simanager.h>
#include <gloox/bytestreamdatahandler.h>
#include <gloox/bytestream.h>

class GlooxMessageHandler;
#include "main.h"



class SendFile : public gloox::BytestreamDataHandler, public Thread {
    std::string m_filename;
    int m_size;
    gloox::Bytestream *m_stream;
	MyMutex *m_mutex;
	FileTransferManager *m_parent;


    std::ifstream m_file;
public:
    SendFile(gloox::Bytestream *stream, std::string filename, int size, MyMutex *mutex, FileTransferManager *manager);

    ~SendFile();

    void exec();

    void handleBytestreamData(gloox::Bytestream *s5b, const std::string &data);

    void handleBytestreamError(gloox::Bytestream *s5b, const gloox::IQ &iq);

    void handleBytestreamOpen(gloox::Bytestream *s5b);

    void handleBytestreamClose(gloox::Bytestream *s5b);
};
#endif
