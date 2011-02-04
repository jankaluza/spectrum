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

#include "filetransfermanager.h"
#include "transport.h"
#include "filetransferrepeater.h"
#include "log.h"
#include "main.h"
#include "abstractuser.h"
#include "usermanager.h"
#include "spectrum_util.h"
#include "abstractuser.h"
#include "abstractspectrumbuddy.h"
#include "utf8.h"
#include "capabilityhandler.h"
#include "Poco/Format.h"

#ifndef TESTS
#include "user.h"
#endif

using namespace gloox;

static void XferCreated(PurpleXfer *xfer) {
	if (xfer) {
		GlooxMessageHandler::instance()->ftManager->handleXferCreated(xfer);
	}
}

static void XferDestroyed(PurpleXfer *xfer) {
	Log("xfer", "xferDestroyed");
}

static void xferCanceled(PurpleXfer *xfer) {
	Log("xfercanceled", xfer);
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	if (!repeater)
		return;
	GlooxMessageHandler::instance()->ftManager->removeSID(repeater->getSID());
	repeater->handleXferCanceled();
}

static void fileSendStart(PurpleXfer *xfer) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->fileSendStart();
	Log("filesend", "fileSendStart()");
}

static void fileRecvStart(PurpleXfer *xfer) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->fileRecvStart();
	Log("filesend", "fileRecvStart()");
}

static void newXfer(PurpleXfer *xfer) {
	Log("purple filetransfer", "new file transfer request from legacy network");
	GlooxMessageHandler::instance()->ftManager->handleXferFileReceiveRequest(xfer);
}

static void XferReceiveComplete(PurpleXfer *xfer) {
	Log("purple filetransfer", "filetransfer receive complete");
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->_tryToDeleteMe();
	GlooxMessageHandler::instance()->ftManager->handleXferFileReceiveComplete(xfer);
	
}

static void XferSendComplete(PurpleXfer *xfer) {
	Log("purple filetransfer", "filetransfer send complete");
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->_tryToDeleteMe();
}

static gssize XferWrite(PurpleXfer *xfer, const guchar *buffer, gssize size) {
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	return repeater->handleLibpurpleData(buffer, size);
}

static void XferNotSent(PurpleXfer *xfer, const guchar *buffer, gsize size) {
	Log("REPEATER", "xferNotSent" << size);
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	repeater->handleDataNotSent(buffer, size);
}

static gssize XferRead(PurpleXfer *xfer, guchar **buffer, gssize size) {
	Log("REPEATER", "xferRead");
	FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
	int data_size = repeater->getDataToSend(buffer, size);
	if (data_size == 0)
		return 0;
	Log("REPEATER", "Passing data to libpurple, size:" << data_size);
	
	return data_size;
}

static PurpleXferUiOps xferUiOps =
{
	XferCreated,
	XferDestroyed,
	NULL,
	NULL,
	xferCanceled,
	xferCanceled,
	XferWrite,
	XferRead,
	XferNotSent,
	NULL
};

FileTransferManager::FileTransferManager() {
	static int xfer_handle;
	purple_signal_connect(purple_xfers_get_handle(), "file-send-start", &xfer_handle, PURPLE_CALLBACK(fileSendStart), NULL);
	purple_signal_connect(purple_xfers_get_handle(), "file-recv-start", &xfer_handle, PURPLE_CALLBACK(fileRecvStart), NULL);
	purple_signal_connect(purple_xfers_get_handle(), "file-recv-request", &xfer_handle, PURPLE_CALLBACK(newXfer), NULL);
	purple_signal_connect(purple_xfers_get_handle(), "file-recv-complete", &xfer_handle, PURPLE_CALLBACK(XferReceiveComplete), NULL);
	purple_signal_connect(purple_xfers_get_handle(), "file-send-complete", &xfer_handle, PURPLE_CALLBACK(XferSendComplete), NULL);
}

FileTransferManager::~FileTransferManager() {
}

void FileTransferManager::setSIProfileFT(gloox::SIProfileFT *sipft) {
	m_sip = sipft;
}

void FileTransferManager::handleFTRequest (const JID &from, const JID &to, const std::string &sid, const std::string &name, long size, const std::string &hash, const std::string &date, const std::string &mimetype, const std::string &desc, int stypes) {
	Log("Received file transfer request from ", from.full() << " " << to.full() << " " << sid << " " << name);
	
	std::string uname = purpleUsername(to.username());

	User *user = Transport::instance()->userManager()->getUserByJID(from.bare());
	if (user && user->account() && user->isConnected()) {
		bool canSendFile = Transport::instance()->canSendFile(user->account(), uname);
		Log("CanSendFile = ", canSendFile);
		Log("filetransferWeb = ", Transport::instance()->getConfiguration().filetransferWeb);
		setTransferInfo(sid, name, size, canSendFile);
		
		m_waitingForXfer[uname].sid = sid;
		m_waitingForXfer[uname].from = from.full();
		m_waitingForXfer[uname].size = size;

		// if we can't send file straightly, we just receive it and send link to buddy.
		if (canSendFile)
			serv_send_file(purple_account_get_connection(user->account()), uname.c_str(), name.c_str());
		else if (!Transport::instance()->getConfiguration().filetransferWeb.empty()) {
			m_repeaters[sid] = new FiletransferRepeater(from.full(), sid, SIProfileFT::FTTypeS5B, to.full(), size);
			m_sip->acceptFT(from, sid, SIProfileFT::FTTypeS5B, to);
		}
		else
			m_sip->declineFT(from, sid, SIManager::RequestRejected , "There is no way how to transfer file.");
	}
}

void FileTransferManager::handleFTBytestream (Bytestream *bs) {
	Log("handleFTBytestream", "target: " << bs->target().full() << " initiator: " << bs->initiator().full());
	if (m_info.find(bs->sid()) != m_info.end()) {
		std::string filename = "";
		// if we will store file, we have to replace invalid characters and prepare directories.
		if (m_info[bs->sid()].straight == false) {
			filename = m_info[bs->sid()].filename;
			// replace invalid characters
			for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
				if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
					*it = '_';
				}
			}
			std::string directory = Transport::instance()->getConfiguration().filetransferCache + "/" + generateUUID();
			purple_build_dir(directory.c_str(), 0755);
			filename = directory + "/" + filename;
		}

		FiletransferRepeater *repeater = m_repeaters[bs->sid()];
		if (!repeater) return;

		if (repeater->isSending())
			repeater->handleFTSendBytestream(bs, filename);
		else
			repeater->handleFTReceiveBytestream(bs, filename);
	}
    m_info.erase(bs->sid());
	m_repeaters.erase(bs->sid());
}

void FileTransferManager::handleXferCreated(PurpleXfer *xfer) {
	User *user = (User *) Transport::instance()->userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (!user) {
		Log("xfercreated", "no user " << xfer->who);
		return;
	}

#ifdef TESTS
	AbstractSpectrumBuddy *buddy = NULL;
#else
	User *user_ = (User *) user;
	AbstractSpectrumBuddy *buddy = user_->getRosterItem(std::string(xfer->who));
#endif

	std::string to;
	// buddy should be always there, but maybe there's network where you can receive file
	// from user who's not in roster...
	if (buddy) {
		to = buddy->getJid();
	}
	else {
		std::string name = std::string(xfer->who);
		size_t pos = name.find("/");
		if (pos != std::string::npos)
			name.erase((int) pos, name.length() - (int) pos);
		name = JID::escapeNode(name);
		to = name + "@" + Transport::instance()->jid() + "/bot";
	}

	FiletransferRepeater *repeater;
	if (purple_xfer_get_type(xfer) == PURPLE_XFER_SEND) {
		std::string key(xfer->who);
		if (m_waitingForXfer.find(key) == m_waitingForXfer.end()) {
			// TODO: DO SOMETHING WITH THAT PURPLE_XFER. PROBABLY REJECT IT.
			Log("xfercreated", "We're not waiting new PurpleXfer for key " << key);
			return;
		}
		else {
			WaitingXferData data = m_waitingForXfer[key];
			Log("xfercreated", "sid = " << data.sid);
			repeater = new FiletransferRepeater(data.from, data.sid, SIProfileFT::FTTypeS5B, to, data.size);
			if (m_repeaters.find(data.sid) != m_repeaters.end()) {
				Log("xfercreated", "there's already repeater for this sid: " << data.sid);
			}
			else {
				m_repeaters[data.sid] = repeater;
			}
			m_waitingForXfer.erase(key);
		}
	}
	else {
		repeater = new FiletransferRepeater(to, user->jid() + "/" + user->getResource().name);
	}
	repeater->registerXfer(xfer);
}

void FileTransferManager::handleXferFileReceiveRequest(PurpleXfer *xfer) {
	std::string tempname(purple_xfer_get_filename(xfer));
	std::string remote_user(purple_xfer_get_remote_user(xfer));

	std::string filename;
	filename.resize(tempname.size());

	// replace invalid characters
	utf8::replace_invalid(tempname.begin(), tempname.end(), filename.begin(), '_');
	for (std::string::iterator it = filename.begin(); it != filename.end(); ++it) {
		if (*it == '\\' || *it == '&' || *it == '/' || *it == '?' || *it == '*' || *it == ':') {
			*it = '_';
		}
	}

	User *user = Transport::instance()->userManager()->getUserByAccount(purple_xfer_get_account(xfer));
	if (user != NULL) {
		FiletransferRepeater *repeater = (FiletransferRepeater *) xfer->ui_data;
		if (user->hasFeature(GLOOX_FEATURE_FILETRANSFER) && 
			!user->getSetting<bool>("save_files_on_server") &&
			!Transport::instance()->getConfiguration().filetransferForceDir) {
			// TODO: CHECK IF requestFT could be part of this class...
			std::string sid = repeater->requestFT();
			if (m_repeaters.find(sid) != m_repeaters.end()) {
				Log("handleXferFileReceiveRequest", "there's already repeater for this sid: " << sid);
			}
			else {
				m_repeaters[sid] = repeater;
			}
		}
		else {
			purple_xfer_request_accepted(xfer, std::string(Transport::instance()->getConfiguration().filetransferCache+"/"+remote_user+"-"+Transport::instance()->getId()+"-"+filename).c_str());
		}
	}
}

void FileTransferManager::handleXferFileReceiveComplete(PurpleXfer *xfer) {
	std::string filename(purple_xfer_get_filename(xfer));
	std::string remote_user(purple_xfer_get_remote_user(xfer));
	if (purple_xfer_get_local_filename(xfer)) {
		std::string localname(purple_xfer_get_local_filename(xfer));
		User *user = Transport::instance()->userManager()->getUserByAccount(purple_xfer_get_account(xfer));
		if (user != NULL) {
			if (user->isConnected()) {
				// filename is in form "/full/path/to/file/HASH/file.tld" where "/full/path/to/file/"
				// has to be replaced by filetransferWeb.
				std::vector <std::string> dirs = split(filename, '/');
				std::string url = Transport::instance()->getConfiguration().filetransferWeb;
				std::string basename = dirs.back();
				dirs.pop_back();
				url += dirs.back() + "/";
				url += basename;
				std::string message  = Poco::format(_("Received file '%s'.  You can download it at: %s"), basename, url );
				Message s(Message::Chat, user->jid(), tr(user->getLang(), message));
				s.setFrom(remote_user + "@" + Transport::instance()->jid() + "/bot");
				Transport::instance()->send(s.tag());
			}
			else{
				Log(user->jid(), "purpleFileReceiveComplete called for unconnected user...");
			}
		}
		else
			Log("purple", "purpleFileReceiveComplete called, but user does not exist!!!");
	}
}

void FileTransferManager::sendFile(const std::string &jid, const std::string &from, const std::string &name, const std::string &file) {
    m_sendlist.push_back(jid);
    std::ifstream f;
    f.open(file.c_str(), std::ios::ate);
    if (f) {
        struct stat info;
        if (stat(file.c_str(), &info) != 0 ) {
            Log("FileTransferManager::sendFile", "stat() call failed!");
            return;
        }
		std::cout << "requesting filetransfer " << jid <<" " << file <<" as " << name << " " << info.st_size << "\n";
        std::string sid = m_sip->requestFT(jid, name, info.st_size, EmptyString, EmptyString, EmptyString, EmptyString, SIProfileFT::FTTypeS5B, from);
        m_info[sid].filename = file;
        m_info[sid].size = info.st_size;
        f.close();
    } else {
        Log("FileTransferManager::sendFile", " Couldn't open the file " << file << "!");
    }
}

PurpleXferUiOps * getXferUiOps() {
	return &xferUiOps;
}
