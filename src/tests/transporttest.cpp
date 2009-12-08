#include "transport.h"
#include "main.h"
#include "../usermanager.h"

Transport* Transport::m_pInstance = NULL;

Transport::Transport(const std::string jid) { m_jid = jid; m_pInstance = this;
	m_userManager = new UserManager();
	m_hash = "123";
}

Transport::~Transport() {
	delete m_userManager;
}

void Transport::send(Tag *tag) {
	std::cout << tag->xml();
	delete tag;
}

UserManager *Transport::userManager() {
	return m_userManager;
}

const std::string &Transport::hash() {
	return m_hash;
}