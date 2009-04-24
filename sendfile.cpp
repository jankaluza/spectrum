#include "sendfile.h"

SendFile::SendFile(gloox::SOCKS5Bytestream *stream, std::string filename, int size, MyMutex *mutex, FileTransferManager *manager) {
    std::cout << "SendFile::SendFile" << " Constructor.\n";
    m_stream = stream;
    m_filename = filename;
    m_size = size;
    m_stream->registerSOCKS5BytestreamDataHandler(this);
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

void SendFile::handleSOCKS5Data(gloox::SOCKS5Bytestream *s5b, const std::string &data) {
	std::cout << "socks stream data\n";
}

void SendFile::handleSOCKS5Error(gloox::SOCKS5Bytestream *s5b, gloox::Stanza *stanza) {

}

void SendFile::handleSOCKS5Open(gloox::SOCKS5Bytestream *s5b) {
	std::cout << "socks stream open\n";
}

void SendFile::handleSOCKS5Close(gloox::SOCKS5Bytestream *s5b) {
	std::cout << "socks stream error\n";
}
