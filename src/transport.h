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

#ifndef TRANSPORT_H
#define TRANSPORT_H
#include <iostream>
#include <string>
#include "gloox/tag.h"
#include "gloox/siprofileft.h"
#include "parser.h"
#include "protocols/abstractprotocol.h"

#include "abstractbackend.h"
#include "capabilitymanager.h"
#include "configfile.h"
#include "accountcollector.h"

using namespace gloox;
class UserManager;

#define PROTOCOL() Transport::instance()->protocol()
#define CONFIG() Transport::instance()->getConfiguration()

/*
 * Transport features used to configure transport.
 */
typedef enum { 	TRANSPORT_FEATURE_TYPING_NOTIFY = 1,
				TRANSPORT_FEATURE_AVATARS = 2,
				TRANSPORT_FEATURE_FILETRANSFER = 4,
				TRANSPORT_FEATURE_STATISTICS = 8,
				TRANSPORT_FEATURE_XSTATUS = 16,
				TRANSPORT_FEATURE_ALL = 255
				} TransportFeatures;


class Transport : public CapabilityManager {
	public:
		Transport(const std::string jid);
		~Transport();
		static Transport *instance() { return m_pInstance; }
		static void send(Tag *tag);
		static void send(IQ &iq, IqHandler *ih, int context, bool del=false);
		static void removeIDHandler(IqHandler *ih);
		static UserManager *userManager();
		const std::string &hash();
		static AbstractBackend *sql();
		const std::string &jid() { return m_jid; }
		static std::string getId();
		std::list <Tag *> &getTags() { return m_tags; }
		void clearTags() { m_tags.clear(); }
		static GlooxParser *parser();
		static AbstractProtocol *protocol();
		static Configuration &getConfiguration();
		static void registerStanzaExtension(StanzaExtension *extension);
		static bool canSendFile(PurpleAccount *account, const std::string &uname);
		static void disposeBytestream(Bytestream *stream);
		static const std::string requestFT( const JID& to, const std::string& name, long size, const std::string& hash = EmptyString, const std::string& desc = EmptyString, const std::string& date = EmptyString, const std::string& mimetype = EmptyString, int streamTypes = SIProfileFT::FTTypeAll, const JID& from = JID(), const std::string& sid = EmptyString );
		static void acceptFT( const JID& to, const std::string& sid, SIProfileFT::StreamType type = SIProfileFT::FTTypeS5B, const JID& from = JID() );
		static AccountCollector *collector();
		bool supportRosterIq() { return m_supportRosterIq; }
		void setSupportRosterIq() { m_supportRosterIq = true; }
		static void fetchVCard(const std::string &jid);
		static bool isAdmin(const std::string &barejid);


		
	private:
		std::string m_jid;
		std::string m_hash;
		std::list <Tag *> m_tags;
		UserManager *m_userManager;
		bool m_supportRosterIq;
		static Transport *m_pInstance;
};

#endif
