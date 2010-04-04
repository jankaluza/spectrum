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

#ifndef _SPECTRUM_FILETRANSFER_REPEATER_H
#define _SPECTRUM_FILETRANSFER_REPEATER_H

#include "purple.h"
#include "localization.h"
#include "gloox/tag.h"
#include "gloox/presence.h"
#include "gloox/siprofileft.h"
#include "gloox/bytestream.h"
#include "gloox/bytestreamdatahandler.h"
#include "conversation.h"
#include "ft.h"
#include "thread.h"
#include <fstream>

class AbstractUser;

extern Localization localization;

using namespace gloox;

class FiletransferRepeater;

class ReceiveFile : public BytestreamDataHandler, public Thread{
	public:
		ReceiveFile(Bytestream *stream, int size, const std::string &filename, AbstractUser *user, FiletransferRepeater *manager);
		~ReceiveFile();

		bool receive();
		void handleBytestreamData(Bytestream *s5b, const std::string &data);
		void handleBytestreamError(Bytestream *s5b, const IQ &iq);
		void handleBytestreamOpen(Bytestream *s5b);
		void handleBytestreamClose(Bytestream *s5b);

		const std::string &filename() { return m_filename; }
		AbstractUser *user () { return m_user; }
		const std::string &target() { return m_target; }
		void dispose();

	private:
		Bytestream *m_stream;
		std::string m_filename;
		std::string m_target;
		int m_size;
		bool m_finished;
		AbstractUser *m_user;
		FiletransferRepeater *m_parent;
		std::ofstream m_file;
};

class ReceiveFileStraight : public BytestreamDataHandler, public Thread {
	public:
		ReceiveFileStraight(Bytestream *stream, FiletransferRepeater *manager);
		~ReceiveFileStraight();

		bool receive();
		void handleBytestreamData(Bytestream *s5b, const std::string &data);
		void handleBytestreamError(Bytestream *s5b, const IQ &iq);
		void handleBytestreamOpen(Bytestream *s5b);
		void handleBytestreamClose(Bytestream *s5b);

		void gotData(const std::string &data);

	private:
		Bytestream *m_stream;
		std::string m_filename;
		bool m_finished;
		FiletransferRepeater *m_parent;
		std::ofstream m_file;
};

class SendFile : public BytestreamDataHandler, public Thread {
	public:
		SendFile(Bytestream *stream, int size, const std::string &filename, AbstractUser *user, FiletransferRepeater *manager);
		~SendFile();

		bool send();
		void handleBytestreamData(Bytestream *s5b, const std::string &data);
		void handleBytestreamError(Bytestream *s5b, const IQ &iq);
		void handleBytestreamOpen(Bytestream *s5b);
		void handleBytestreamClose(Bytestream *s5b);

		const std::string &filename() { return m_filename; }
		AbstractUser *user() { return m_user; }
		void dispose();

	private:
		Bytestream *m_stream;
		std::string m_filename;
		AbstractUser *m_user;
		int m_size;
		FiletransferRepeater *m_parent;
		std::ofstream m_file;
};

class SendFileStraight : public BytestreamDataHandler, public Thread {
	public:
		SendFileStraight(Bytestream *stream, int size, FiletransferRepeater *manager);
		~SendFileStraight();

		void exec();
		bool send();
		void handleBytestreamData(Bytestream *s5b, const std::string &data);
		void handleBytestreamError(Bytestream *s5b, const IQ &iq);
		void handleBytestreamOpen(Bytestream *s5b);
		void handleBytestreamClose(Bytestream *s5b);

	private:
		Bytestream *m_stream;
		std::string m_filename;
		int m_size;
		FiletransferRepeater *m_parent;
};

// Class which acts as repeater between Gloox and libpurple.
class FiletransferRepeater {
	public:
		// Constructor for XMPP -> Legacy network transfer.
		FiletransferRepeater(const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size);

		// Constructor for Legacy network -> XMPP transfer.
		FiletransferRepeater(const JID& from, const JID& to);

		~FiletransferRepeater();

		// Registers PurpleXfer with FiletransferRepeater.
		void registerXfer(PurpleXfer *xfer);

		// Called when libpurple cancels filetransfer.
		void handleXferCanceled();

		// Called when libpurple starts sending file to legacy network.
		void fileSendStart();

		// Called when libpurple starts receiving file from legacy network.
		void fileRecvStart();

		// Called when Gloox has opened bytestream to receive file from XMPP.
		void handleFTReceiveBytestream(Bytestream *bs, const std::string &filename = "");

		// Called when Gloox has opened bytestream to send file over XMPP.
		void handleFTSendBytestream(Bytestream *bs, const std::string &filename = "");

		// Called by resender thread when it has new data.
		// Returns true if FiletransferRepeater doesn't want more data for now, thread will sleep
		// until it's waked up.
		bool handleGlooxData(const std::string &data);

		// Called by libpurple when it has new data. Returns the size of really handled data.
		gssize handleLibpurpleData(const guchar *data, gssize size);

		// Handles data which couldn't be sent by libpurple. They should be sent in next libpurple
		// request.
		void handleDataNotSent(const guchar *data, gssize size);

		// Copies data which will be passed to libpurple and sent to legacy network into 'data'.
		// Returns size of 'data'.
		int getDataToSend(guchar **data, gssize size);

		// Copies data which will be sent by Gloox over XMPP into 'data'. Returns size of 'data'.
		int getDataToSend(std::string &data);

		// Sends filetransfer request to XMPP user.
		std::string requestFT();

		// Returns true if we're sending file from legacy network to XMPP.
		bool isSending() { return m_send; }

		// Sets UI ready for the transfer.
		void ready();

		// Returns filetransfer SID.
		const std::string &getSID() { return m_sid; }

		// Callback which tells libpurple that UI is ready.
		// MUST be called only by timer.
		void ui_ready_callback();

		// Callback which tries to delete this class.
		// MUST be called only by timer.
		void tryToDeleteMe();

	private:
		JID m_to;
		std::string m_sid;
		SIProfileFT::StreamType m_type;
		JID m_from;
		long m_size;
		PurpleXfer *m_xfer;
		bool m_send;
		bool m_readyCalled;
		guint m_readyTimer;
		guchar *m_buffer;
		size_t m_buffer_size;
		size_t m_max_buffer_size;
		Thread *m_resender;

};

#endif
