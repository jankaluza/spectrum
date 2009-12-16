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
	Transport::instance()->clearTags();
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

void RosterManagerTest::generatePresenceStanza() {
	return;
	
	setRoster();
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	Tag *tag;
	
	tag = m_buddy1->generatePresenceStanza(TRANSPORT_FEATURE_AVATARS);
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findChild("show") == NULL);
	CPPUNIT_ASSERT (tag->findChild("status") != NULL);
	CPPUNIT_ASSERT (tag->findChild("status")->cdata() == "I'm here");
	CPPUNIT_ASSERT (tag->findChild("x") != NULL);
	CPPUNIT_ASSERT (tag->findChild("x")->findAttribute("xmlns") == "vcard-temp:x:update");
	CPPUNIT_ASSERT (tag->findChild("x")->findChild("photo") != NULL);
	CPPUNIT_ASSERT (tag->findChild("x")->findChild("photo")->cdata() == "somehash");

	tag = m_buddy1->generatePresenceStanza(0);
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findChild("show") == NULL);
	CPPUNIT_ASSERT (tag->findChild("status") != NULL);
	CPPUNIT_ASSERT (tag->findChild("status")->cdata() == "I'm here");
	CPPUNIT_ASSERT (tag->findChild("x") == NULL);
	
	m_buddy1->setStatus(PURPLE_STATUS_AWAY);
	tag = m_buddy1->generatePresenceStanza(0);
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findChild("status") != NULL);
	CPPUNIT_ASSERT (tag->findChild("status")->cdata() == "I'm here");
	CPPUNIT_ASSERT (tag->findChild("show") != NULL);
	CPPUNIT_ASSERT (tag->findChild("show")->cdata() == "away");
	CPPUNIT_ASSERT (tag->findChild("x") == NULL);
}


void RosterManagerTest::sendUnavailablePresenceToAll() {
	m_manager->sendUnavailablePresenceToAll();
	m_tags = Transport::instance()->getTags();
	
	CPPUNIT_ASSERT (m_tags.size() == 0);
	
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

void RosterManagerTest::sendPresenceToAll() {
	setRoster();
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_buddy2->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy2->setStatusMessage("I'm here");
	m_buddy2->setIconHash("somehash");
	m_manager->sendPresenceToAll("user@example.com");
	m_tags = Transport::instance()->getTags();

	std::list <std::string> users;
	users.push_back("user1%example.com@icq.localhost/bot");
	users.push_back("user2%example.com@icq.localhost/bot");

	CPPUNIT_ASSERT_MESSAGE ("Nothing was sent #1", m_tags.size() != 0);
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence for user who is not in roster",
								find(users.begin(), users.end(), tag->findAttribute("from")) != users.end());
		users.remove(tag->findAttribute("from"));
		
		
		m_tags.remove(tag);
		delete tag;
	}
	CPPUNIT_ASSERT_MESSAGE ("Presence for one or more users from roster was not sent", users.size() == 0);
	Transport::instance()->clearTags();
	
}

void RosterManagerTest::isInRoster() {
	setRoster();
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	m_manager->removeFromLocalRoster("user1@example.com");
	CPPUNIT_ASSERT (!m_manager->isInRoster("user1@example.com", ""));
}
