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

#include "log.h"

LogMessage::LogMessage(std::ofstream &file, bool newline) : m_file(file), m_newline(newline) {
}

LogMessage::~LogMessage() {
	if (m_newline)
		os << std::endl;
	fprintf(stdout, "%s", os.str().c_str());
	fflush(stdout);
	if (m_file.is_open()) {
		m_file << os.str();
		m_file.flush();
	}
}

std::ostringstream &LogMessage::Get(const std::string &user) {
	time_t now = time(0);
	char timestamp_buf[25] = "";
	strftime (timestamp_buf, 25, "%x %H:%M:%S", localtime(&now));
	std::string timestamp = std::string(timestamp_buf);
	os << "[" << timestamp << "] <" << user << "> ";
	return os;
}

LogClass::LogClass() {
	g_thread_init(NULL);
	m_mutex = g_mutex_new();
}

LogClass::~LogClass() {
	if (m_file.is_open())
		m_file.close();
	g_mutex_free(m_mutex);
}

void LogClass::setLogFile(const std::string &file) {
	if (m_file.is_open())
		m_file.close();
	m_file.open(file.c_str(), std::ios_base::app);
	g_chmod(file.c_str(), 0640);
}

std::ofstream &LogClass::fileStream() {
	return m_file;
}

void LogClass::handleLog(LogLevel level, LogArea area, const std::string &message) {
	if (area == LogAreaXmlIncoming)
		log(get("XML IN") << message);
	else
		log(get("XML OUT") << message);
}

std::ostringstream &LogClass::get(const std::string &user, bool newline) {
	g_mutex_lock(m_mutex);
	os.clear();
	os.str("");
	m_newline = newline;
	time_t now = time(0);
	char timestamp_buf[25] = "";
	strftime (timestamp_buf, 25, "%x %H:%M:%S", localtime(&now));
	std::string timestamp = std::string(timestamp_buf);
	os << "[" << timestamp << "] <" << user << "> ";
	return os;
}

void LogClass::log(void *data) {
	if (m_newline)
		os << std::endl;
	fprintf(stdout, "%s", os.str().c_str());
	fflush(stdout);
	if (m_file.is_open()) {
		m_file << os.str();
		m_file.flush();
	}
	os.clear();
	g_mutex_unlock(m_mutex);
}


LogClass Log_;
