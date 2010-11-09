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

extern LogClass Log_;

enum {
	PING = 0,
	PONG,
	LOGIN
};

class SpectrumBackendMessage {
	public:
		SpectrumBackendMessage() {}
		SpectrumBackendMessage(int t) : type(t), int1(0) {}
		SpectrumBackendMessage(int t, int i1) : type(t), int1(i1) {}
		SpectrumBackendMessage(int t, const std::string &s1) : type(t), str1(s1), int1(0) {}
		int type;
		std::string str1;
		int int1;

		void send(boost::shared_ptr<Swift::Connection> connection) {
			std::ostringstream ss;
			boost::archive::binary_oarchive oa(ss);
			oa & *this;

			unsigned long size = ss.str().size();
			Swift::ByteArray data((const char *) &size, sizeof(unsigned long));
			data += Swift::ByteArray(ss.str().data(), size);
			connection->write(data);
		}

		void send(GIOChannel *source) {
			std::ostringstream ss;
			boost::archive::binary_oarchive oa(ss);
			oa & *this;

			unsigned long size = ss.str().size();
			gsize bytes;
			std::cout << "sending " << int1 << " " << ss.str().data() << "\n";
			g_io_channel_write_chars(source, (const char *) &size, sizeof(unsigned long), &bytes, NULL);
			g_io_channel_write_chars(source, ss.str().data(), size, &bytes, NULL);
			g_io_channel_flush (source, NULL);
		}

	private:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & type;
			ar & str1;
			ar & int1;
		}
};

class SpectrumPurple {
	public:
		SpectrumPurple(boost::asio::io_service* ioService);
		~SpectrumPurple();

	private:
		void handleConnected(bool error);
		void initPurple();
	
		bool m_parent;
		
};

#endif
