#include "rosterstoragetest.h"
#include "rosterstorage.h"
#include "transport.h"

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
}

void RosterStorageTest::addRoster() {
 	m_storage->storeBuddy(m_buddy1);
	m_storage->storeBuddy(m_buddy2);
}
