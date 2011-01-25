#include "transport.h"
#include "main.h"
#include "usermanager.h"
#include "filetransfermanager.h"
#include "adhoc/adhochandler.h"
#include "spectrumtimer.h"
#include "spectrumdiscohandler.h"
#include "gatewayhandler.h"
#include <fcntl.h>
#include <signal.h>
#include "utf8.h"
#include "log.h"
#include "geventloop.h"
#include "accountcollector.h"
#include "autoconnectloop.h"
#include "dnsresolver.h"
#include "usermanager.h"
#include "adhoc/adhochandler.h"
#include "adhoc/adhocrepeater.h"
#ifndef WIN32
#include "configinterface.h"
#endif
#include "searchhandler.h"
#include "searchrepeater.h"
#include "filetransferrepeater.h"
#include "registerhandler.h"
#include "spectrumdiscohandler.h"
#include "statshandler.h"
#include "vcardhandler.h"
#include "gatewayhandler.h"
#include "capabilityhandler.h"
#include "configfile.h"
#include "spectrum_util.h"
#include "sql.h"
#include "user.h"
#include "protocolmanager.h"
#include "filetransfermanager.h"
#include "localization.h"
#include "spectrumbuddy.h"
#include "spectrumnodehandler.h"
#include "transport.h"

#include "gloox/adhoc.h"
#include "gloox/error.h"
#include <gloox/tlsbase.h>
#include <gloox/compressionbase.h>
#include <gloox/mucroom.h>
#include <gloox/sha.h>
#include <gloox/vcardupdate.h>
#include <gloox/base64.h>
#include <gloox/chatstate.h>

#ifdef WITH_LIBEVENT
#include <event.h>
#endif

Transport* Transport::m_pInstance = NULL;

static gboolean _pingTimerTimeout(gpointer data) {
	Transport::instance()->sendPing();
	return TRUE;
}

static void transportDataReceived(gpointer data, gint source, PurpleInputCondition cond) {
	Transport::instance()->receiveData();
}

static gboolean ftServerReceive(gpointer data) {
	if (Transport::instance()->ftServerReceiveData() == ConnNoError)
		return TRUE;
	return FALSE;
}

/*
 * Connect user to legacy network
 */
static gboolean connectUser(gpointer data) {
	std::string name((char*)data);
	User *user = (User *) Transport::instance()->userManager()->getUserByJID(name);
	if (user && user->readyForConnect() && !user->isConnected()) {
		user->connect();
	}
	g_free(data);
	return FALSE;
}

/*
 * Reconnect user
 */
static gboolean reconnect(gpointer data) {
	std::string name((char*)data);
	User *user = (User *) Transport::instance()->userManager()->getUserByJID(name);
	if (user) {
		user->connect();
	}
	g_free(data);
	return FALSE;
}

static gboolean transportReconnect(gpointer data) {
	Transport::instance()->transportConnect();
	return FALSE;
}

Transport::Transport(Configuration c, AbstractProtocol *protocol) : MessageHandler(),ConnectionListener(),PresenceHandler(),SubscriptionHandler()  {
	m_configuration = c;
	m_protocol = protocol;
	m_jid = c.jid;
	m_pInstance = this;
	m_reconnectCount = 0;
	m_firstConnection = true;
	m_socketId = 0;
	m_searchHandler = NULL;

	m_component = new Component("jabber:component:accept", m_configuration.server, m_configuration.jid, m_configuration.password, m_configuration.port);
	m_adhoc = new GlooxAdhocHandler(m_component->disco());

	m_pingTimer = new SpectrumTimer(60000, &_pingTimerTimeout, this);
	m_userManager = new UserManager();
	
	m_parser = new GlooxParser();
	m_collector = new AccountCollector();

	m_component->registerIqHandler(m_adhoc, ExtAdhocCommand);
	m_component->registerStanzaExtension( new Adhoc::Command() );
	m_component->registerStanzaExtension( new MUCRoom::MUC() );
	m_component->registerStanzaExtension(new ChatState(ChatStateInvalid));
	m_component->registerStanzaExtension( new RosterExtension() );
	m_component->disco()->addFeature( XMLNS_ADHOC_COMMANDS );
	m_component->disco()->registerNodeHandler( m_adhoc, XMLNS_ADHOC_COMMANDS );
	m_component->disco()->registerNodeHandler( m_adhoc, std::string() );
	m_component->removeIqHandler( m_component->disco(), ExtDiscoInfo );
	m_component->removeIqHandler( m_component->disco(), ExtDiscoItems );
	m_discoHandler = new SpectrumDiscoHandler(m_component->disco());
	m_component->registerIqHandler(m_discoHandler, ExtDiscoInfo);
	m_component->registerIqHandler(m_discoHandler, ExtDiscoItems);
	m_component->registerIqHandler(this, 1055);
	m_capabilityHandler = new CapabilityHandler();

	m_component->registerMessageHandler(this);
	m_component->registerConnectionListener(this);
	m_gatewayHandler = new GlooxGatewayHandler();
	m_component->registerIqHandler(m_gatewayHandler, ExtGateway);
	m_reg = new GlooxRegisterHandler();
	m_component->registerIqHandler(m_reg, ExtRegistration);
	m_stats = new GlooxStatsHandler();
	m_component->registerIqHandler(m_stats, ExtStats);
	m_vcardManager = new VCardManager(m_component);
#ifndef WIN32
	if (m_configInterface)
		m_configInterface->registerHandler(m_stats);
#endif
	m_vcard = new GlooxVCardHandler();
	m_component->registerIqHandler(m_vcard, ExtVCard);
	m_component->registerPresenceHandler(this);
	m_component->registerSubscriptionHandler(this);
	Transport::instance()->registerStanzaExtension( new VCardUpdate );
	m_spectrumNodeHandler = new SpectrumNodeHandler();


	m_ftManager = new FileTransferManager();
	m_siProfileFT = new SIProfileFT(m_component, m_ftManager);
	m_ftManager->setSIProfileFT(m_siProfileFT);
	m_ftServer = new SOCKS5BytestreamServer(m_component->logInstance(), CONFIG().filetransfer_proxy_port, CONFIG().filetransfer_proxy_ip);
	if( m_ftServer->listen() != ConnNoError ) {}
	m_siProfileFT->addStreamHost( m_jid, CONFIG().filetransfer_proxy_streamhost_ip, CONFIG().filetransfer_proxy_streamhost_port);
	m_siProfileFT->registerSOCKS5BytestreamServer( m_ftServer );
	if (CONFIG().transportFeatures & TRANSPORT_FEATURE_FILETRANSFER) {
		purple_timeout_add(500, &ftServerReceive, NULL);
	}

	if (CONFIG().logAreas & LOG_AREA_PURPLE)
		m_component->logInstance().registerLogHandler(LogLevelDebug, LogAreaXmlIncoming | LogAreaXmlOutgoing, &Log_);
#ifndef WIN32
	if (!CONFIG().config_interface.empty())
		m_configInterface = new ConfigInterface(CONFIG().config_interface, m_component->logInstance());
#endif
}

Transport::~Transport() {
	delete m_component;
	delete m_pingTimer;
	delete m_adhoc;
	delete m_discoHandler;
}

void Transport::send(Tag *tag) {
	Transport::instance()->m_component->send(tag);
}

void Transport::send(IQ &iq, IqHandler *ih, int context, bool del) {
	Transport::instance()->m_component->send(iq, ih, context, del);
}

UserManager *Transport::userManager() {
	return m_userManager;
}

const std::string &Transport::hash() {
	return m_configuration.hash;
}

AbstractBackend *Transport::sql() {
	return m_sql;
}

std::string Transport::getId() {
	return m_component->getID();
}

GlooxParser *Transport::parser() {
	return m_parser;
}

AbstractProtocol *Transport::protocol() {
	return m_protocol;
}

Configuration &Transport::getConfiguration() {
	return m_configuration;
}

void Transport::registerStanzaExtension(StanzaExtension *extension) {
	return m_component->registerStanzaExtension(extension);
}

bool Transport::canSendFile(PurpleAccount *account, const std::string &uname) {
	PurplePlugin *prpl = NULL;
	PurplePluginProtocolInfo *prpl_info = NULL;
	PurpleConnection *gc = purple_account_get_connection(account);
	if (gc)
		prpl = purple_connection_get_prpl(gc);

	if (prpl)
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	if (prpl_info && prpl_info->send_file) {
		if (prpl_info->can_receive_file && prpl_info->can_receive_file(gc, uname.c_str())) {
			return true;
		}
	}
	return false;
}

void Transport::disposeBytestream(Bytestream *stream) {
	m_siProfileFT->dispose(stream);
}

const std::string Transport::requestFT( const JID& to, const std::string& name, long size, const std::string& hash, const std::string& desc, const std::string& date, const std::string& mimetype, int streamTypes, const JID& from, const std::string& sid) {
	std::string r_sid = m_siProfileFT->requestFT(to, name, size, hash, desc, date, mimetype, streamTypes, from, sid);
	m_ftManager->setTransferInfo(r_sid, name, size, true);
	return r_sid;
}

void Transport::acceptFT( const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from ) {
	m_siProfileFT->acceptFT(to, sid, type, from);
}

AccountCollector *Transport::collector() {
	return m_collector;
}

void Transport::fetchVCard(const std::string &jid) { Transport::instance()->fetchVCard(jid); }

bool Transport::isAdmin(const std::string &bare_jid) {
	std::list<std::string> const &admins = Transport::instance()->getConfiguration().admins;
	return (std::find(admins.begin(), admins.end(), bare_jid) != admins.end());
}

void Transport::disconnect() {
	m_component->disconnect();
}

void Transport::receiveData() {
	m_component->recv(1000);
}

int Transport::ftServerReceiveData() {
	return m_ftServer->recv(1);
}

void Transport::handlePresence(const Presence &stanza){
	if (stanza.subtype() == Presence::Error) {
		return;
	}

	if (!isValidNode(stanza.from().username())) {
		SpectrumRosterManager::sendErrorPresence(stanza.from().full(), stanza.to().full(), "jid-malformed");
		return;
	}
	
	// get entity capabilities
	Tag *c = NULL;
	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
	Log(stanza.from().full(), "Presence received (" << (int)stanza.subtype() << ") for: " << stanza.to().full() << "isMUC" << isMUC);


	if (stanza.presence() != Presence::Unavailable && ((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || (isMUC && protocol()->tempAccountsAllowed()))) {
		Tag *stanzaTag = stanza.tag();
		if (!stanzaTag) return;
		Tag *c = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/caps");
		Log(stanza.from().full(), "asking for caps/disco#info");
		// Presence has caps and caps are not cached.
		if (c != NULL && !Transport::instance()->hasClientCapabilities(c->findAttribute("ver"))) {
			int context = m_capabilityHandler->waitForCapabilities(c->findAttribute("ver"), stanza.to().full());
			std::string node = c->findAttribute("node") + std::string("#") + c->findAttribute("ver");;
			m_component->disco()->getDiscoInfo(stanza.from(), node, m_capabilityHandler, context, getId());
		}
		else {
			int context = m_capabilityHandler->waitForCapabilities(stanza.from().full(), stanza.to().full());
			m_component->disco()->getDiscoInfo(stanza.from(), "", m_capabilityHandler, context, getId());
		}
		delete stanzaTag;
	}
	
	User *user;
	std::string userkey;
	if (protocol()->tempAccountsAllowed()) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		userkey = stanza.from().bare() + server;
		user = (User *) userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = (User *) userManager()->getUserByJID(stanza.from().bare());
		userkey = stanza.from().bare();
	}
	if (user == NULL) {
		// we are not connected and probe arrived => answer with unavailable
		if (stanza.subtype() == Presence::Probe) {
			Log(stanza.from().full(), "Answering to probe presence with unavailable presence");
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", stanza.from().full());
			tag->addAttribute("from", stanza.to().bare());
			tag->addAttribute("type", "unavailable");
			Transport::instance()->send(tag);
			if (stanza.to().username() == "") {
				Tag *s = new Tag("presence");
				s->addAttribute( "to", stanza.from().bare());
				s->addAttribute( "type", "probe");
				s->addAttribute( "from", jid());
				Transport::instance()->send(s);
			}
		}
		else if (((stanza.to().username() == "" && !protocol()->tempAccountsAllowed()) || ( protocol()->tempAccountsAllowed() && isMUC)) && stanza.presence() != Presence::Unavailable){
			UserRow res = sql()->getUserByJid(userkey);
			if(res.id==-1 && !protocol()->tempAccountsAllowed()) {
				// presence from unregistered user
				Log(stanza.from().full(), "This user is not registered");
				return;
			}
			else {
				if(res.id==-1 && protocol()->tempAccountsAllowed()) {
					res.jid = userkey;
					res.uin = stanza.from().username();
					res.password = "";
					res.language = "en";
					res.encoding = m_configuration.encoding;
					res.vip = 0;
					sql()->addUser(res);
					res = sql()->getUserByJid(userkey);
				}
				bool isVip = res.vip;
				std::list<std::string> const &x = m_configuration.allowedServers;
				if (m_configuration.onlyForVIP && !isVip && std::find(x.begin(), x.end(), stanza.from().server()) == x.end()) {
					Log(stanza.from().full(), "This user is not VIP, can't login...");
					return;
				}
				Log(stanza.from().full(), "Creating new User instance");
				if (protocol()->tempAccountsAllowed()) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					user = new User(stanza.from(), stanza.to().resource() + "@" + server, "", stanza.from().bare() + server, res.id, res.encoding, res.language, res.vip);
				}
				else {
					if (purple_accounts_find(res.uin.c_str(), protocol()->protocol().c_str()) != NULL) {
						PurpleAccount *act = purple_accounts_find(res.uin.c_str(), protocol()->protocol().c_str());
						user = (User *) userManager()->getUserByAccount(act);
						if (user) {
							Log(stanza.from().full(), "This account is already connected by another jid " << user->jid());
							return;
						}
					}
					user = new User(stanza.from(), res.uin, res.password, stanza.from().bare(), res.id, res.encoding, res.language, res.vip);
				}
				user->setFeatures(isVip ? m_configuration.VIPFeatures : m_configuration.transportFeatures);
				if (c != NULL)
					if (Transport::instance()->hasClientCapabilities(c->findAttribute("ver")))
						user->setResource(stanza.from().resource(), stanza.priority(), Transport::instance()->getCapabilities(c->findAttribute("ver")));

				m_userManager->addUser(user);
				user->receivedPresence(stanza);
				if (protocol()->tempAccountsAllowed()) {
					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
					server = stanza.from().bare() + server;
					purple_timeout_add_seconds(15, &connectUser, g_strdup(server.c_str()));
				}
				else
					purple_timeout_add_seconds(15, &connectUser, g_strdup(stanza.from().bare().c_str()));
			}
		}
		if (stanza.presence() == Presence::Unavailable && stanza.to().username() == ""){
			Log(stanza.from().full(), "User is already logged out => sending unavailable presence");
			Tag *tag = new Tag("presence");
			tag->addAttribute( "to", stanza.from().bare() );
			tag->addAttribute( "type", "unavailable" );
			tag->addAttribute( "from", jid() );
			Transport::instance()->send( tag );
		}
	}
	else {
		user->receivedPresence(stanza);
	}
	if (stanza.to().username() == "" && user != NULL) {
		if(stanza.presence() == Presence::Unavailable && user->isConnected() == true && user->getResources().empty()) {
			Log(stanza.from().full(), "Logging out");
			m_userManager->removeUser(user);
		}
		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false && user->getResources().empty()) {
			Log(stanza.from().full(), "Logging out, but he's not connected...");
			m_userManager->removeUser(user);
		}
// 		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false) {
// 			Log(stanza.from().full(), "Can't logout because we're connecting now...");
// 		}
	}
	else if (user != NULL && stanza.presence() == Presence::Unavailable && PROTOCOL()->tempAccountsAllowed() && !user->hasOpenedMUC()) {
		m_userManager->removeUser(user);
	}
	else if (user == NULL && stanza.to().username() == "" && stanza.presence() == Presence::Unavailable) {
		UserRow res = sql()->getUserByJid(userkey);
		if (res.id != -1) {
			sql()->setUserOnline(res.id, false);
		}
	}
}


void Transport::onConnect() {
	Log("gloox", "CONNECTED!");
	m_reconnectCount = 0;

	if (m_firstConnection) {
		m_component->disco()->setIdentity("gateway", protocol()->gatewayIdentity(), m_configuration.discoName);
		m_discoHandler->setIdentity("client", "pc", "Spectrum");
		m_component->disco()->setVersion(m_configuration.discoName, VERSION, "");

		std::string id = "client";
		id += '/';
		id += "pc";
		id += '/';
		id += '/';
		id += "Spectrum";
		id += '<';
		
		std::list<std::string> features = protocol()->transportFeatures();
		features.sort();
		for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
			// these features are default in gloox
			if (*it != "http://jabber.org/protocol/disco#items" &&
				*it != "http://jabber.org/protocol/disco#info" &&
				*it != "http://jabber.org/protocol/commands") {
				m_component->disco()->addFeature(*it);
			}
		}

		std::list<std::string> f = protocol()->buddyFeatures();
		if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#items") == f.end())
			f.push_back("http://jabber.org/protocol/disco#items");
		if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#info") == f.end())
			f.push_back("http://jabber.org/protocol/disco#info");
		f.sort();
		for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
			id += (*it);
			id += '<';
			m_discoHandler->addFeature(*it);
		}

		SHA sha;
		sha.feed( id );
		m_configuration.hash = Base64::encode64( sha.binary() );
		m_component->disco()->registerNodeHandler( m_spectrumNodeHandler, "http://spectrum.im/transport#" + m_configuration.hash );
		m_discoHandler->registerNodeHandler( m_spectrumNodeHandler, "http://spectrum.im/transport#" + m_configuration.hash );

		if (m_configuration.protocol != "irc")
			new AutoConnectLoop();
		m_firstConnection = false;
		m_pingTimer->start();
// 		purple_timeout_add_seconds(60, &sendPing, this);
	}
}

void Transport::onDisconnect(ConnectionError e) {
	Log("gloox", "Disconnected from Jabber server !");
	switch (e) {
		case ConnNoError: Log("gloox", "Reason: No error"); break;
		case ConnStreamError: Log("gloox", "Reason: Stream error"); break;
		case ConnStreamVersionError: Log("gloox", "Reason: Stream version error"); break;
		case ConnStreamClosed: Log("gloox", "Reason: Stream closed"); break;
		case ConnProxyAuthRequired: Log("gloox", "Reason: Proxy auth required"); break;
		case ConnProxyAuthFailed: Log("gloox", "Reason: Proxy auth failed"); break;
		case ConnProxyNoSupportedAuth: Log("gloox", "Reason: Proxy no supported auth"); break;
		case ConnIoError: Log("gloox", "Reason: IO Error"); break;
		case ConnParseError: Log("gloox", "Reason: Parse error"); break;
		case ConnConnectionRefused: Log("gloox", "Reason: Connection refused"); break;
// 		case ConnSocketError: Log("gloox", "Reason: Socket error"); break;
		case ConnDnsError: Log("gloox", "Reason: DNS Error"); break;
		case ConnOutOfMemory: Log("gloox", "Reason: Out Of Memory"); break;
		case ConnNoSupportedAuth: Log("gloox", "Reason: No supported auth"); break;
		case ConnTlsFailed: Log("gloox", "Reason: Tls failed"); break;
		case ConnTlsNotAvailable: Log("gloox", "Reason: Tls not available"); break;
		case ConnCompressionFailed: Log("gloox", "Reason: Compression failed"); break;
// 		case ConnCompressionNotAvailable: Log("gloox", "Reason: Compression not available"); break;
		case ConnAuthenticationFailed: Log("gloox", "Reason: Authentication Failed"); break;
		case ConnUserDisconnected: Log("gloox", "Reason: User disconnected"); break;
		case ConnNotConnected: Log("gloox", "Reason: Not connected"); break;
	};

	switch (m_component->streamError()) {
		case StreamErrorBadFormat: Log("gloox", "Stream error: Bad format"); break;
		case StreamErrorBadNamespacePrefix: Log("gloox", "Stream error: Bad namespace prefix"); break;
		case StreamErrorConflict: Log("gloox", "Stream error: Conflict"); break;
		case StreamErrorConnectionTimeout: Log("gloox", "Stream error: Connection timeout"); break;
		case StreamErrorHostGone: Log("gloox", "Stream error: Host gone"); break;
		case StreamErrorHostUnknown: Log("gloox", "Stream error: Host unknown"); break;
		case StreamErrorImproperAddressing: Log("gloox", "Stream error: Improper addressing"); break;
		case StreamErrorInternalServerError: Log("gloox", "Stream error: Internal server error"); break;
		case StreamErrorInvalidFrom: Log("gloox", "Stream error: Invalid from"); break;
		case StreamErrorInvalidId: Log("gloox", "Stream error: Invalid ID"); break;
		case StreamErrorInvalidNamespace: Log("gloox", "Stream error: Invalid Namespace"); break;
		case StreamErrorInvalidXml: Log("gloox", "Stream error: Invalid XML"); break;
		case StreamErrorNotAuthorized: Log("gloox", "Stream error: Not Authorized"); break;
		case StreamErrorPolicyViolation: Log("gloox", "Stream error: Policy violation"); break;
		case StreamErrorRemoteConnectionFailed: Log("gloox", "Stream error: Remote connection failed"); break;
		case StreamErrorResourceConstraint: Log("gloox", "Stream error: Resource constraint"); break;
		case StreamErrorRestrictedXml: Log("gloox", "Stream error: Restricted XML"); break;
		case StreamErrorSeeOtherHost: Log("gloox", "Stream error: See other host"); break;
		case StreamErrorSystemShutdown: Log("gloox", "Stream error: System shutdown"); break;
		case StreamErrorUndefinedCondition: Log("gloox", "Stream error: Undefined Condition"); break;
		case StreamErrorUnsupportedEncoding: Log("gloox", "Stream error: Unsupported encoding"); break;
		case StreamErrorUnsupportedStanzaType: Log("gloox", "Stream error: Unsupported stanza type"); break;
		case StreamErrorUnsupportedVersion: Log("gloox", "Stream error: Unsupported version"); break;
		case StreamErrorXmlNotWellFormed: Log("gloox", "Stream error: XML Not well formed"); break;
		case StreamErrorUndefined: Log("gloox", "Stream error: Error undefined"); break;
	};

	switch (m_component->authError()) {
		case AuthErrorUndefined: Log("gloox", "Auth error: Error undefined"); break;
		case SaslAborted: Log("gloox", "Auth error: Sasl aborted"); break;
		case SaslIncorrectEncoding: Log("gloox", "Auth error: Sasl incorrect encoding"); break;        
		case SaslInvalidAuthzid: Log("gloox", "Auth error: Sasl invalid authzid"); break;
		case SaslInvalidMechanism: Log("gloox", "Auth error: Sasl invalid mechanism"); break;
		case SaslMalformedRequest: Log("gloox", "Auth error: Sasl malformed request"); break;
		case SaslMechanismTooWeak: Log("gloox", "Auth error: Sasl mechanism too weak"); break;
		case SaslNotAuthorized: Log("gloox", "Auth error: Sasl Not authorized"); break;
		case SaslTemporaryAuthFailure: Log("gloox", "Auth error: Sasl temporary auth failure"); break;
		case NonSaslConflict: Log("gloox", "Auth error: Non sasl conflict"); break;
		case NonSaslNotAcceptable: Log("gloox", "Auth error: Non sasl not acceptable"); break;
		case NonSaslNotAuthorized: Log("gloox", "Auth error: Non sasl not authorized"); break;
	};

	if (m_reconnectCount == 2)
		m_userManager->removeAllUsers();

	Log("gloox", "trying to reconnect after 1 second");
	if (m_socketId > 0) {
		purple_input_remove(m_socketId);
		m_socketId = 0;
	}
	purple_timeout_add_seconds(1, &transportReconnect, NULL);

// 	if (connectIO) {
// 		g_source_remove(connectID);
// 		connectIO = NULL;
// 	}
}

void Transport::transportConnect() {
	m_reconnectCount++;
	if (m_sql->loaded()) {
		m_component->connect(false);
		int mysock = dynamic_cast<ConnectionTCPClient*>( m_component->connectionImpl() )->socket();
		if (mysock > 0) {
			if (m_socketId > 0)
				purple_input_remove(m_socketId);
			m_socketId = purple_input_add(mysock, PURPLE_INPUT_READ, &transportDataReceived, NULL);
// 			connectIO = g_io_channel_unix_new(mysock);
// 			connectID = g_io_add_watch(connectIO, (GIOCondition) READ_COND, &transportDataReceived, NULL);
		}
	}
	else {
		Log("gloox", "Tried to reconnect, but database is not ready yet.");
		Log("gloox", "trying to reconnect after 1 second");
		purple_timeout_add_seconds(1, &transportReconnect, NULL);
	}
}

bool Transport::onTLSConnect(const CertInfo & info) {
	return false;
}

void Transport::handleMessage (const Message &msg, MessageSession *session) {
	if (msg.from().bare() == msg.to().bare())
		return;
	if (msg.subtype() == Message::Error || msg.subtype() == Message::Invalid)
		return;
	
	User *user;
	// TODO; move it to IRCProtocol
	if (m_configuration.protocol == "irc") {
		std::string server = msg.to().username().substr(msg.to().username().find("%") + 1, msg.to().username().length() - msg.to().username().find("%"));
		user = (User *) userManager()->getUserByJID(msg.from().bare() + server);
	}
	else {
		user = (User *) userManager()->getUserByJID(msg.from().bare());
	}
	if (user!=NULL) {
		if (user->isConnected()) {
			Tag *msgTag = msg.tag();
			if (!msgTag) return;
			const StanzaExtension *ext = msg.findExtension(ExtChatState);
			Tag *chatstates = ext ? ext->tag() : NULL;
			if (chatstates != NULL) {
// 				std::string username = msg.to().username();
// 				std::for_each( username.begin(), username.end(), replaceJidCharacters() );
				user->handleChatState(purpleUsername(msg.to().username()), chatstates->name());
				delete chatstates;
			}
			if (msgTag->findChild("body") != NULL) {
				m_stats->messageFromJabber();
				user->handleMessage(msg);
			}
			delete msgTag;
		}
		else if (msg.to().username() == "") {
			Transport::instance()->protocol()->onXMPPMessageReceived(user, msg);
		}
		else{
			Log(msg.from().bare(), "New message received, but we're not connected yet");
		}
	}
	else {
		Tag *msgTag = msg.tag();
		if (!msgTag) return;
		if (msgTag->findChild("body") != NULL) {
			Message s(Message::Error, msg.from().full(), msg.body());
			s.setFrom(msg.to().full());
			Error *c = new Error(StanzaErrorTypeWait, StanzaErrorRecipientUnavailable);
			c->setText(tr(m_configuration.language.c_str(),_("This message couldn't be sent, because you are not connected to legacy network. You will be automatically reconnected soon.")));
			s.addExtension(c);
			Transport::instance()->send(s.tag());

			Tag *stanza = new Tag("presence");
			stanza->addAttribute( "to", msg.from().bare());
			stanza->addAttribute( "type", "probe");
			stanza->addAttribute( "from", jid());
			Transport::instance()->send(stanza);
		}
		delete msgTag;
	}

}

void Transport::handleVCard(const JID& jid, const VCard* vcard) {
	if (!vcard)
		return;
	User *user = (User *) Transport::instance()->userManager()->getUserByJID(jid.bare());
	if (user && user->isConnected()) {
		user->handleVCard(vcard);
	}
}

void Transport::handleVCardResult(VCardContext context, const JID& jid, StanzaError se) {
	
}

bool Transport::handleIq (const IQ &iq) {
	Tag *tag = iq.tag();
	if (!tag)
		return true;
	User *user = (User *) Transport::instance()->userManager()->getUserByJID(iq.from().bare());
	if (user) {
		user->handleRosterResponse(tag);
	}
	delete tag;
	return true;
}

void Transport::handleIqID (const IQ &iq, int context) {
	
}


void Transport::handleSubscription(const Subscription &stanza) {
	// answer to subscibe
	if(stanza.subtype() == Subscription::Subscribe && stanza.to().username() == "") {
		Log(stanza.from().full(), "Subscribe presence received => sending subscribed");
		Tag *reply = new Tag("presence");
		reply->addAttribute( "to", stanza.from().bare() );
		reply->addAttribute( "from", stanza.to().bare() );
		reply->addAttribute( "type", "subscribed" );
		Transport::instance()->send( reply );
		return;
	}

	if (CONFIG().protocol == "irc") {
		return;
	}

	User *user;
	if (protocol()->tempAccountsAllowed()) {
		std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
		user = (User *) userManager()->getUserByJID(stanza.from().bare() + server);
	}
	else {
		user = (User *) userManager()->getUserByJID(stanza.from().bare());
	}
	if (user)
		user->handleSubscription(stanza);
	else if (stanza.subtype() == Subscription::Unsubscribe) {
		Tag *tag = new Tag("presence");
		tag->addAttribute("to", stanza.from().bare());
		tag->addAttribute("from", stanza.to().username() + "@" + Transport::instance()->jid());
		tag->addAttribute( "type", "unsubscribed" );
		Transport::instance()->send( tag );
	}
	else
		Log(stanza.from().full(), "Subscribe presence received, but this user is not logged in");

}

void Transport::setProtocol(AbstractProtocol *protocol) {
	m_protocol = protocol;
	if (!m_protocol->userSearchAction().empty()) {
		m_searchHandler = new GlooxSearchHandler();
		m_component->registerIqHandler(m_searchHandler, ExtSearch);
	}
	
	if (m_configuration.encoding.empty())
		m_configuration.encoding = m_protocol->defaultEncoding();

	ConfigFile cfg(m_configuration.file);
	cfg.loadPurpleAccountSettings(m_configuration);
}
