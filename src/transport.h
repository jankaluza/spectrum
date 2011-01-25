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
#include "gloox/component.h"
#include <gloox/messagehandler.h>
#include <gloox/connectiontcpclient.h>
#include <gloox/disco.h>
#include <gloox/connectionlistener.h>
#include <gloox/presencehandler.h>
#include <gloox/subscriptionhandler.h>
#include <gloox/socks5bytestreamserver.h>
#include <gloox/siprofileft.h>
#include <gloox/message.h>
#include <gloox/eventhandler.h>
#include <gloox/vcardhandler.h>
#include <gloox/vcardmanager.h>
#include "parser.h"
#include "protocols/abstractprotocol.h"

#include "abstractbackend.h"
#include "capabilitymanager.h"
#include "configfile.h"
#include "accountcollector.h"

using namespace gloox;
class UserManager;
class GlooxAdhocHandler;
class ConfigInterface;
class SpectrumDiscoHandler;
class GlooxGatewayHandler;
class CapabilityHandler;
class SpectrumDiscoHandler;
class GlooxRegisterHandler;
class GlooxXPingHandler;
class GlooxStatsHandler;
class GlooxVCardHandler;
class GlooxGatewayHandler;
class GlooxAdhocHandler;
class SQLClass;
class FileTransferManager;
class UserManager;
class AbstractProtocol;
class AdhocRepeater;
class GlooxSearchHandler;
class GlooxParser;
class AccountCollector;
class Transport;
class SpectrumNodeHandler;

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

// Class representing whole Transport object :).
class Transport : public CapabilityManager, public MessageHandler, ConnectionListener, PresenceHandler, SubscriptionHandler, LogHandler, public EventHandler, public VCardHandler, public IqHandler {
	public:
		Transport(Configuration cfg, AbstractProtocol *protocol);
		~Transport();
		static Transport *instance() { return m_pInstance; }

		// Sends tag to server.
		void send(Tag *tag);

		// Sends IQ to server. IqHandler is called with 'context' when response is received.
		// if del is true, then IqHandler is deleted that (god bless gloox...).
		void send(IQ &iq, IqHandler *ih, int context, bool del=false);

		// Returns pointer to UserManager class.
		UserManager *userManager();

		// Returns hash computed from features of contacts representing legacy network
		// buddies.
		const std::string &hash();

		// Returns pointer to used sql backend.
		AbstractBackend *sql();
		void setSQLBackend(AbstractBackend *sql) { m_sql = sql; }

		// Returns transport's JID.
		const std::string &jid() { return m_jid; }

		// Returns ID which can be used as 'id' for IQs.
		std::string getId();

		// Returns pointer to xml parser.
		GlooxParser *parser();

		// Returns pointer to currently used protocol class.
		AbstractProtocol *protocol();

		// Returns configuration structure.
		Configuration &getConfiguration();

		// Registers new gloox StanzaExtension.
		void registerStanzaExtension(StanzaExtension *extension);

		// Returns true transport can send file to this buddy.
		// TODO: Rename me as canReceiveFile... it's more logical.
		bool canSendFile(PurpleAccount *account, const std::string &uname);

		// Closes socket and frees gloox ByteStream.
		void disposeBytestream(Bytestream *stream);

		// Sends filetransfer request.
		const std::string requestFT( const JID& to, const std::string& name, long size, const std::string& hash = EmptyString, const std::string& desc = EmptyString, const std::string& date = EmptyString, const std::string& mimetype = EmptyString, int streamTypes = SIProfileFT::FTTypeAll, const JID& from = JID(), const std::string& sid = EmptyString );

		// Accepts filetransfer request.
		void acceptFT( const JID& to, const std::string& sid, SIProfileFT::StreamType type = SIProfileFT::FTTypeS5B, const JID& from = JID() );

		// Returns pointer to accountCollector class.
		AccountCollector *collector();

		// Fetches VCard.
		void fetchVCard(const std::string &jid);

		// Returns true if particular user id admin of this transport.
		bool isAdmin(const std::string &barejid);

		// Disconnects this transport from Jabber server.
		void disconnect();

		// Sends ping to Jabber server.
		void sendPing() {  m_component->xmppPing(m_jid, this); }

		// Reads data from Jabber server.
		void receiveData();
		int ftServerReceiveData();

		void setConfiguration(const Configuration &cfg) {m_configuration = cfg;}
	void transportConnect();
	void onConnect();
	void onDisconnect(ConnectionError e);
	void onSessionCreateError(const Error *error) {}
	bool onTLSConnect(const CertInfo & info);
	
	void handleMessage (const Message &msg, MessageSession *session = 0);
	void handlePresence(const Presence &presence);
	void handleSubscription(const Subscription &stanza);
	void handleLog(LogLevel level, LogArea area, const std::string &message) {}
	void handleEvent (const Event &event) {}
	void handleVCard(const JID& jid, const VCard* vcard);
	void handleVCardResult(VCardContext context, const JID& jid, StanzaError se);

	bool handleIq (const IQ &iq);
	void handleIqID (const IQ &iq, int context);
	void setProtocol(AbstractProtocol *protocol);

		// Used for automatic tests. TODO: remove me?
		std::list <Tag *> &getTags() { return m_tags; }
		void clearTags() { m_tags.clear(); }

		
	private:
		AbstractBackend *m_sql;
		std::string m_jid;
		std::string m_hash;
		std::list <Tag *> m_tags;
		UserManager *m_userManager;
		static Transport *m_pInstance;
		Component *m_component;
		SpectrumTimer *m_pingTimer;
		GlooxAdhocHandler *m_adhoc;
		SpectrumDiscoHandler *m_discoHandler;
		SIProfileFT* m_siProfileFT;
		GlooxGatewayHandler *m_gatewayHandler;
		GlooxRegisterHandler *m_reg;
		GlooxStatsHandler *m_stats;
		GlooxVCardHandler *m_vcard;
		GlooxSearchHandler *m_searchHandler;
		VCardManager* m_vcardManager;
		FileTransferManager* m_ftManager;
		SOCKS5BytestreamServer* m_ftServer;
		Configuration m_configuration;
		CapabilityHandler *m_capabilityHandler;
		AbstractProtocol *m_protocol;
		int m_reconnectCount;
		GlooxParser *m_parser;						// Gloox parser - makes Tag* from std::string
		AccountCollector *m_collector;
		bool m_firstConnection;
		SpectrumNodeHandler *m_spectrumNodeHandler;
		guint m_socketId;
#ifndef WIN32
		ConfigInterface *m_configInterface;
#endif
};

#endif
