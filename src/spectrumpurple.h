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

#ifndef SPECTRUM_PURPLE_H
#define SPECTRUM_PURPLE_H

#include <vector>
#include "glib.h"

#include "Swiften/Network/BoostConnection.h"
#include "log.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "spectrumconnection.h"

extern LogClass Log_;

enum {
	MSG_PING = 0,
	MSG_PONG,
	MSG_UUID,
	MSG_CONNECTED,
	MSG_CHANGE_STATUS,
	MSG_LOGOUT,
	MSG_LOGIN
};

class SpectrumBackendMessage {
	public:
		SpectrumBackendMessage() {}
		SpectrumBackendMessage(int t) : type(t), int1(0) {}
		SpectrumBackendMessage(int t, int i1) : type(t), int1(i1) {}
		SpectrumBackendMessage(int t, const std::string &s1) : type(t), str1(s1), int1(0) {}
		int type;
		std::string str1;
		std::string str2;
		std::string str3;
		std::string str4;
		int int1;

		void send(GIOChannel *source) {
			std::ostringstream ss;
			boost::archive::binary_oarchive oa(ss);
			oa & *this;

			std::ostringstream header_stream;
			header_stream << std::setw(8) << std::hex << ss.str().size();

			gsize bytes;
			g_io_channel_write_chars(source, header_stream.str().data(), header_stream.str().size(), &bytes, NULL);
			g_io_channel_write_chars(source, ss.str().data(), ss.str().size(), &bytes, NULL);
			g_io_channel_flush (source, NULL);
		}

	private:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & type;
			ar & str1;
			ar & str2;
			ar & str3;
			ar & str4;
			ar & int1;
		}
};

class SpectrumPurple {
	public:
		SpectrumPurple(boost::asio::io_service* ioService);
		~SpectrumPurple();

		void login(const std::string &uin, const std::string &passwd, int status, const std::string &message);
		void changeStatus(const std::string &uin, int status, const std::string &message);
		void logout(const std::string &uin);

		const std::string &getUUID() { return m_uuid; }
		void setUUID(const std::string &uuid) { m_uuid = uuid; }

		const bool &isLogged() { return m_logged; }
		void setLogged(const bool &logged) { m_logged = logged; }

		void setConnection(connection_ptr conn) { m_conn = conn; }

	private:
		void handleConnected(bool error);
		void handleInstanceWrite(const boost::system::error_code& e, connection_ptr conn) { }
		void initPurple();
	
		bool m_parent;
		std::string m_uuid;
		bool m_logged;
		connection_ptr m_conn;
		
};

#endif
