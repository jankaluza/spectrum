#include "transport.h"
#include "main.h"
#include "usermanager.h"
#include "filetransfermanager.h"

Transport* Transport::m_pInstance = NULL;

Transport::Transport(const std::string jid) { m_jid = jid; m_pInstance = this; }

Transport::~Transport() {}

void Transport::send(Tag *tag) {
	GlooxMessageHandler::instance()->j->send(tag);
}

void Transport::send(IQ &iq, IqHandler *ih, int context, bool del) {
	GlooxMessageHandler::instance()->j->send(iq, ih, context, del);
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
	return GlooxMessageHandler::instance()->j->getID();
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
	return GlooxMessageHandler::instance()->j->registerStanzaExtension(extension);
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
