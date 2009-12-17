#include "rosterstoragetest.h"
#include "rosterstorage.h"
#include "transport.h"
#include "testingbackend.h"

void RosterStorageTest::setUp (void) {
	m_buddy1 = new SpectrumBuddyTest(-1, NULL);
	m_buddy1->setName("user1@example.com");
	m_buddy1->setAlias("Frank");
	m_buddy1->setOnline();
	
	m_buddy2 = new SpectrumBuddyTest(1, NULL);
	m_buddy2->setName("user2@example.com");
	m_buddy2->setAlias("Bob");
	m_buddy2->setOnline();
	
	m_user = new TestingUser("key", "user@example.com");
	m_storage = new RosterStorage(m_user);
}

void RosterStorageTest::tearDown (void) {
	delete m_buddy1;
	delete m_buddy2;
	delete m_storage;
	delete m_user;
	
	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	backend->reset();
}

void RosterStorageTest::storeBuddies() {
 	m_storage->storeBuddy(m_buddy1);
	m_storage->storeBuddy(m_buddy2);

	CPPUNIT_ASSERT (m_storage->storeBuddies());

	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	std::map <std::string, Buddy> buddies = backend->getBuddies();

	CPPUNIT_ASSERT (buddies.find("user1@example.com") != buddies.end());
	CPPUNIT_ASSERT (buddies.find("user2@example.com") != buddies.end());

}

void RosterStorageTest::storeBuddiesRemove() {
 	m_storage->storeBuddy(m_buddy1);
	m_storage->removeBuddy(m_buddy1);
	m_storage->storeBuddy(m_buddy2);

	CPPUNIT_ASSERT (m_storage->storeBuddies());

	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	std::map <std::string, Buddy> buddies = backend->getBuddies();

	CPPUNIT_ASSERT (buddies.find("user1@example.com") == buddies.end());
	CPPUNIT_ASSERT (buddies.find("user2@example.com") != buddies.end());

}
