#include "rostermanagertest.h"
#include "rostermanager.h"
#include "transport.h"


void RosterManagerTest::setUp (void) {
	m_buddy1 = new SpectrumBuddyTest(1, NULL);
	m_buddy1->setName("user1@example.com");
	m_buddy1->setAlias("Frank");
	m_buddy1->setOnline();
	
	m_buddy2 = new SpectrumBuddyTest(2, NULL);
	m_buddy2->setName("user2@example.com");
	m_buddy2->setAlias("Bob");
	m_buddy2->setOnline();
	
	m_user = new TestingUser("key", "user@example.com");
	m_manager = new RosterManager(m_user);
}

void RosterManagerTest::tearDown (void) {
	delete m_buddy1;
	delete m_buddy2;
	delete m_manager;
	delete m_user;
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		m_tags.remove(tag);
		delete tag;
	}
}

void RosterManagerTest::setRoster() {
	GHashTable *roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	g_hash_table_replace(roster, g_strdup(m_buddy1->getName().c_str()), m_buddy1);
	g_hash_table_replace(roster, g_strdup(m_buddy2->getName().c_str()), m_buddy2);
	m_manager->setRoster(roster);
	CPPUNIT_ASSERT(m_manager->getRosterItem(m_buddy1->getName()) == m_buddy1);
	CPPUNIT_ASSERT(m_manager->getRosterItem(m_buddy2->getName()) == m_buddy2);
	CPPUNIT_ASSERT(m_manager->getRosterItem("something") == NULL);
	
}

void RosterManagerTest::sendUnavailablePresenceToAll() {
	setRoster();
	m_manager->sendUnavailablePresenceToAll();
	m_tags = Transport::instance()->getTags();

	std::list <std::string> users;
	users.push_back("user1%example.com@icq.localhost/bot");
	users.push_back("user2%example.com@icq.localhost/bot");

	CPPUNIT_ASSERT_MESSAGE ("Nothing was sent #1", m_tags.size() != 0);
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "unavailable");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence for user who is not in roster",
								find(users.begin(), users.end(), tag->findAttribute("from")) != users.end());
		users.remove(tag->findAttribute("from"));
		
		
		m_tags.remove(tag);
		delete tag;
	}
	CPPUNIT_ASSERT_MESSAGE ("Presence for one or more users from roster was not sent", users.size() == 0);
	
	m_buddy1->setOffline();
	m_buddy2->setOnline();
	Transport::instance()->clearTags();
	m_manager->sendUnavailablePresenceToAll();
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("Nothing was sent #2", m_tags.size() != 0);

	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "unavailable");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence sent, but it's nof for m_buddy2",
								tag->findAttribute("from") == "user2%example.com@icq.localhost/bot");
		users.remove(tag->findAttribute("from"));

		m_tags.remove(tag);
		delete tag;
	}

	
	
}
