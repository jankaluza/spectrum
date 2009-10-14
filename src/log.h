#ifndef _HI_LOG_H
#define _HI_LOG_H

#include <time.h>
#include <sstream>
#include <cstdio>

class Log
{
public:
	Log(){};
	virtual ~Log() {
		os << std::endl;
		fprintf(stdout, "%s", os.str().c_str());
		fflush(stdout);
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
};
#endif
