#include "autoconnectlooptest.h"
#include "autoconnectloop.h"
#include "transport.h"
#include "testingbackend.h"
#include "../capabilityhandler.h"

void AutoConnectLoopTest::up (void) {
	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	backend->addOnlineUser("user1@example.com");
	backend->addOnlineUser("user2@example.com");
	m_loop = new AutoConnectLoop();
}

void AutoConnectLoopTest::down (void) {
	delete m_loop;
}

void AutoConnectLoopTest::restoreNextConnection() {
	std::string r1;
	std::string r2;
	r1 =	"<presence from='icq.localhost' type='probe' to='user1@example.com'/>";
	r2 =	"<presence from='icq.localhost' type='probe' to='user2@example.com'/>";
	
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == true);
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == true);
	testTagCount(2);
	compare(r1);
	compare(r2);
	
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == false);
	testTagCount(2);
	compare(r1);
}
