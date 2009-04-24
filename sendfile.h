
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
