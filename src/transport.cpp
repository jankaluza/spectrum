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

SQLClass *Transport::sql() {
	return GlooxMessageHandler::instance()->sql();
}
