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

class AbstractResendClass {
	public:
		AbstractResendClass() {
			m_mutex = g_mutex_new();
			m_cond = g_cond_new();
			m_stop = false;
			m_thread = NULL;
		}
		virtual ~AbstractResendClass() { g_mutex_free(m_mutex); g_cond_free(m_cond); }

		GMutex *getMutex() { return m_mutex; }
		void wait() {
			g_cond_wait(m_cond, m_mutex);
		}

		void wakeUp() {
			g_cond_signal(m_cond);
		}
		
		void stop() {
			g_mutex_lock(m_mutex);
			m_stop = true;
			g_mutex_unlock(m_mutex);
		}
		
		void execute(GThreadFunc func, gpointer data) {
			m_thread = g_thread_create(func, data, true, NULL);
		}
		
		void join() {
			if(m_thread)
				g_thread_join(m_thread);
		}

		bool isRunning() {
			return m_thread != NULL;
		}

	bool m_stop;
	GThread *m_thread;

	private:
		GMutex *m_mutex;
		GCond *m_cond;
		
};

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
		ReceiveFileStraight(Bytestream *stream, int size, FiletransferRepeater *manager);
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
		int m_size;
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

class FiletransferRepeater {

	public:
		FiletransferRepeater(const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size);
		FiletransferRepeater(const JID& from, const JID& to);
		~FiletransferRepeater() {}

		void registerXfer(PurpleXfer *xfer);
		void fileSendStart();
		void fileRecvStart();
		void handleFTReceiveBytestream(Bytestream *bs, const std::string &filename = "");
		void handleFTSendBytestream(Bytestream *bs, const std::string &filename = "");
		bool gotData(const std::string &data);
		gssize handleLibpurpleData(const guchar *data, gssize size);
		void handleDataNotSent(const guchar *data, gssize size);
		int getDataToSend(guchar **data, gssize size);
		int getDataToSend(std::string &data);
		void xferDestroyed();
		std::string requestFT();

		bool isSending() { return m_send; }

		std::string & getBuffer() { return a; }
		void wantsData() { m_wantsData = true; if (m_resender) m_resender->wakeUp(); }
		void ready();
		std::ofstream m_file;

	private:
		JID m_to;
		std::string m_sid;
		SIProfileFT::StreamType m_type;
		JID m_from;
		long m_size;
		PurpleXfer *m_xfer;
		bool m_send;
		std::string a;
		bool m_readyCalled;
		guint m_readyTimer;


		guchar *m_buffer;
		size_t m_buffer_size;
		size_t m_max_buffer_size;
		Thread *m_resender;
		bool m_wantsData;

};

#endif
