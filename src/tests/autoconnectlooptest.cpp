#include "autoconnectlooptest.h"
#include "autoconnectloop.h"
#include "transport.h"
#include "testingbackend.h"
#include "../capabilityhandler.h"

void AutoConnectLoopTest::up (void) {
	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	backend->addOnlineUser("user1@example.com");
	backend->addOnlineUser("user1@example.com");
	m_loop = new AutoConnectLoop();
}

void AutoConnectLoopTest::down (void) {
	delete m_loop;
}

void AutoConnectLoopTest::restoreNextConnection() {
	std::string r;
	r =	"<presence from='icq.localhost' type='probe' to='user1@example.com'/>";
	
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == true);
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == true);
	testTagCount(2);
	
	CPPUNIT_ASSERT (m_loop->restoreNextConnection() == false);
	testTagCount(2);
	compare(r);
}
