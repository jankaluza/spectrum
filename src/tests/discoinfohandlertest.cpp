#include "discoinfohandlertest.h"
#include "discoinfohandler.h"
#include "transport.h"
#include "testingbackend.h"

void GlooxDiscoInfoHandlerTest::setUp (void) {
	m_handler = new GlooxDiscoInfoHandler();
}

void GlooxDiscoInfoHandlerTest::tearDown (void) {
	delete m_handler;
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		m_tags.remove(tag);
		delete tag;
	}
	Transport::instance()->clearTags();
}

Tag *GlooxDiscoInfoHandlerTest::getCorrectTag(const std::string &to) {
	Tag *iq = new Tag("iq");
	iq->addAttribute("type", "get");
	iq->addAttribute("from", "user@example.com/psi");
	iq->addAttribute("to", to);
	iq->addAttribute("id", "id1");
	
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "http://jabber.org/protocol/disco#info");
	
	iq->addChild(query);
	return iq;
}

void GlooxDiscoInfoHandlerTest::handleIq() {
	Tag *iq = getCorrectTag("buddy1@icq.localhost");
	m_handler->handleIq(iq);
	m_tags = Transport::instance()->getTags();
	
	CPPUNIT_ASSERT (m_tags.size() == 1);
}
