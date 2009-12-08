#include "rostermanagertest.h"
#include "rostermanager.h"


void RosterManagerTest::setUp (void) {
	m_buddy1 = new SpectrumBuddyTest(1, NULL);
	m_buddy1->setName("user1@example.com");
	m_buddy1->setAlias("Frank");
	
	m_buddy2 = new SpectrumBuddyTest(2, NULL);
	m_buddy2->setName("user2@example.com");
	m_buddy2->setAlias("Bob");
	
	m_user = new TestingUser("key", "user@example.com");
	m_manager = new RosterManager(m_user);
}

void RosterManagerTest::tearDown (void) {
	delete m_buddy1;
	delete m_buddy2;
	delete m_manager;
	delete m_user;
}

void RosterManagerTest::setRoster (void) {
	GHashTable *roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	g_hash_table_replace(roster, g_strdup(m_buddy1->getName().c_str()), m_buddy1);
	g_hash_table_replace(roster, g_strdup(m_buddy2->getName().c_str()), m_buddy2);
	m_manager->setRoster(roster);
	CPPUNIT_ASSERT(m_manager->getRosterItem(m_buddy1->getName()) == m_buddy1);
	CPPUNIT_ASSERT(m_manager->getRosterItem(m_buddy2->getName()) == m_buddy2);
	CPPUNIT_ASSERT(m_manager->getRosterItem("something") == NULL);
	
}

