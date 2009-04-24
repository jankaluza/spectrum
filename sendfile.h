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
// #include <gloox/siprofileft.h>
// #include <gloox/siprofilefthandler.h>
#include <gloox/socks5bytestreammanager.h>
#include <gloox/socks5bytestreamdatahandler.h>

class GlooxMessageHandler;
#include "main.h"



class SendFile : public gloox::SOCKS5BytestreamDataHandler, public Thread {
    std::string m_filename;
    int m_size;
    gloox::SOCKS5Bytestream *m_stream;
	MyMutex *m_mutex;
	FileTransferManager *m_parent;


    std::ifstream m_file;
public:
    SendFile(gloox::SOCKS5Bytestream *stream, std::string filename, int size, MyMutex *mutex, FileTransferManager *manager);

    ~SendFile();

    void exec();

    void handleSOCKS5Data(gloox::SOCKS5Bytestream *s5b, const std::string &data);

    void handleSOCKS5Error(gloox::SOCKS5Bytestream *s5b, gloox::Stanza *stanza);

    void handleSOCKS5Open(gloox::SOCKS5Bytestream *s5b);

    void handleSOCKS5Close(gloox::SOCKS5Bytestream *s5b);
};
#endif
