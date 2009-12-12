#ifndef _HI_LOG_H
#define _HI_LOG_H

#include <time.h>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include "gloox/loghandler.h"

using namespace gloox;

class LogClass;

class LogMessage
{
public:
	LogMessage(std::ofstream &file) : m_file(file) {}
	~LogMessage() {
		os << std::endl;
		fprintf(stdout, "%s", os.str().c_str());
		fflush(stdout);
		if (m_file.is_open()) {
			m_file << os.str();
			m_file.flush();
		}
	}

	std::ostringstream& Get(const std::string &user) {
		time_t now = time(0);
		char timestamp_buf[25] = "";
		strftime (timestamp_buf, 25, "%x %H:%M:%S", localtime(&now));
		std::string timestamp = std::string(timestamp_buf);
		os << "[" << timestamp << "] <" << user << "> ";
		return os;
	}
protected:
	std::ostringstream os;
	std::ofstream &m_file;
};

class LogClass : public LogHandler {
	public:
		LogClass() {}
		~LogClass() {
			if (m_file.is_open())
				m_file.close();
		}

		void setLogFile(const std::string &file) {
			if (m_file.is_open())
				m_file.close();
			m_file.open(file.c_str());
		}
		
		std::ofstream &fileStream() { return m_file; }
		void handleLog(LogLevel level, LogArea area, const std::string &message) {
			if (area == LogAreaXmlIncoming)
				LogMessage(m_file).Get("XML IN") << message;
			else
				LogMessage(m_file).Get("XML OUT") << message;
		}
		
	private:
		std::ofstream m_file;
};
#ifdef WITHOUT_LOGS
	#define Log(HEAD,STRING) 
#else
	#define Log(HEAD,STRING) LogMessage(Log_.fileStream()).Get(HEAD) << STRING;
#endif

#endif
