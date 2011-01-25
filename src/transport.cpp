#include "transport.h"
#include "main.h"
#include "usermanager.h"
#include "filetransfermanager.h"

Transport* Transport::m_pInstance = NULL;

static gboolean _pingTimerTimeout(gpointer data) {
	Transport::instance()->sendPing();
	return TRUE;
}

static void transportDataReceived(gpointer data, gint source, PurpleInputCondition cond) {
	Transport::instance()->receiveData();
}

Transport::Transport(const std::string jid, const std::string &server, const std::string &password, int port) {
	m_jid = jid;
	m_pInstance = this;

	m_component = new Component("jabber:component:accept", server, jid, password, port);
	m_adhoc = new GlooxAdhocHandler(m_component->disco());

	m_pingTimer = new SpectrumTimer(60000, &_pingTimerTimeout, this);

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
	GlooxMessageHandler::instance()->m_component->send(tag);
}

void Transport::send(IQ &iq, IqHandler *ih, int context, bool del) {
	GlooxMessageHandler::instance()->m_component->send(iq, ih, context, del);
}

UserManager *Transport::userManager() {
	return GlooxMessageHandler::instance()->userManager();
}

const std::string &Transport::hash() {
	return GlooxMessageHandler::instance()->configuration().hash;
}

AbstractBackend *Transport::sql() {
	return (AbstractBackend *) GlooxMessageHandler::instance()->sql();
}

std::string Transport::getId() {
	return GlooxMessageHandler::instance()->m_component->getID();
}

GlooxParser *Transport::parser() {
	return GlooxMessageHandler::instance()->parser();
}

AbstractProtocol *Transport::protocol() {
	return GlooxMessageHandler::instance()->protocol();
}

Configuration &Transport::getConfiguration() {
	return GlooxMessageHandler::instance()->configuration();
}

void Transport::registerStanzaExtension(StanzaExtension *extension) {
	return GlooxMessageHandler::instance()->m_component->registerStanzaExtension(extension);
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
	GlooxMessageHandler::instance()->ft->dispose(stream);
}

const std::string Transport::requestFT( const JID& to, const std::string& name, long size, const std::string& hash, const std::string& desc, const std::string& date, const std::string& mimetype, int streamTypes, const JID& from, const std::string& sid) {
	std::string r_sid = GlooxMessageHandler::instance()->ft->requestFT(to, name, size, hash, desc, date, mimetype, streamTypes, from, sid);
	GlooxMessageHandler::instance()->ftManager->setTransferInfo(r_sid, name, size, true);
	return r_sid;
}

void Transport::acceptFT( const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from ) {
	GlooxMessageHandler::instance()->ft->acceptFT(to, sid, type, from);
}

AccountCollector *Transport::collector() {
	return GlooxMessageHandler::instance()->collector();
}

void Transport::fetchVCard(const std::string &jid) { GlooxMessageHandler::instance()->fetchVCard(jid); }

bool Transport::isAdmin(const std::string &bare_jid) {
	std::list<std::string> const &admins = Transport::instance()->getConfiguration().admins;
	return (std::find(admins.begin(), admins.end(), bare_jid) != admins.end());
}

void Transport::disconnect() {
	GlooxMessageHandler::instance()->m_component->disconnect();
}

void Transport::receiveData() {
	m_component->read(1000);
}
