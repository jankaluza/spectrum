#ifndef RECEIVEFILE_H
#define RECEIVEFILE_H
#include "thread.h"

#include <gloox/socks5bytestreamdatahandler.h>
#include <gloox/socks5bytestream.h>
class FileTransferManager;
class GlooxMessageHandler;
#include <iostream>
#include <fstream>
#include <string>


struct progress;

class ReceiveFile : public gloox::SOCKS5BytestreamDataHandler,
                    public Thread {
    gloox::SOCKS5Bytestream *m_stream;
    std::string m_filename;
    int m_size;
    bool m_finished;

	MyMutex *m_mutex;
	FileTransferManager *m_parent;

    std::ofstream m_file;
public:
    ReceiveFile(gloox::SOCKS5Bytestream *stream, std::string filename, int size, MyMutex *mutex, FileTransferManager *manager);

    ~ReceiveFile();

    void exec();

    void handleSOCKS5Data(gloox::SOCKS5Bytestream *s5b, const std::string &data);

    void handleSOCKS5Error(gloox::SOCKS5Bytestream *s5b, gloox::Stanza *stanza);

    void handleSOCKS5Open(gloox::SOCKS5Bytestream *s5b);

    void handleSOCKS5Close(gloox::SOCKS5Bytestream *s5b);
};

#endif
