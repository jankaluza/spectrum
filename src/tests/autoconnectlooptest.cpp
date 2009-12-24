#include "autoconnectlooptest.h"
#include "autoconnectloop.h"
#include "transport.h"
#include "testingbackend.h"
#include "../capabilityhandler.h"

void AutoConnectLoopTest::setUp (void) {
	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	backend->addOnlineUser("user1@example.com");
	backend->addOnlineUser("user1@example.com");
	m_loop = new AutoConnectLoop();
}

void AutoConnectLoopTest::tearDown (void) {
	delete m_loop;
	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	backend->clearOnlineUsers();
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		m_tags.remove(tag);
		delete tag;
	}
	Transport::instance()->clearTags();
}

void AutoConnectLoopTest::restoreNextConnection() {
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == true);
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 presence stanza sent", m_tags.size() == 1);
	
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == true);
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 2 presence stanza sent", m_tags.size() == 2);
	
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == false);
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 2 presence stanza sent", m_tags.size() == 2);
	
	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT( m_tags.front()->xml() == m_tags.back()->xml());
	
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "probe");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user1@example.com");
}
