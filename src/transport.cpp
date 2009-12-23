#include "transport.h"
#include "main.h"
#include "usermanager.h"

Transport* Transport::m_pInstance = NULL;

Transport::Transport(const std::string jid) { m_jid = jid; m_pInstance = this; }

Transport::~Transport() {}

void Transport::send(Tag *tag) {
	GlooxMessageHandler::instance()->j->send(tag);
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

