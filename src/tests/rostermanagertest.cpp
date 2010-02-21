#include "rostermanagertest.h"
#include "rostermanager.h"
#include "transport.h"


void RosterManagerTest::up (void) {
	m_buddy1 = new SpectrumBuddyTest(1, NULL);
	m_buddy1->setName("user1@example.com");
	m_buddy1->setAlias("Frank");
	m_buddy1->setOnline();
	
	m_buddy2 = new SpectrumBuddyTest(2, NULL);
	m_buddy2->setName("user2@example.com");
	m_buddy2->setAlias("Bob");
	m_buddy2->setOnline();
	
	m_user = new TestingUser("key", "user@example.com");
	m_manager = new SpectrumRosterManager(m_user);
}

void RosterManagerTest::down (void) {
	delete m_buddy1;
	delete m_buddy2;
	delete m_manager;
	delete m_user;
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
	setRoster();
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	Tag *tag;
	
	std::string r;
	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<status>I'm here</status>"
			"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
			"<x xmlns='vcard-temp:x:update'>"
				"<photo>somehash</photo>"
			"</x>"
		"</presence>";
	tag = m_buddy1->generatePresenceStanza(TRANSPORT_FEATURE_AVATARS);
	compare(tag, r);
	clearTags();

	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
			"<status>I'm here</status>"
		"</presence>";
	tag = m_buddy1->generatePresenceStanza(0);
	compare(tag, r);
	clearTags();

	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<show>away</show>"
			"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
			"<status>I'm here</status>"
		"</presence>";
	m_buddy1->setStatus(PURPLE_STATUS_AWAY);
	tag = m_buddy1->generatePresenceStanza(0);
	compare(tag, r);
	clearTags();

	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
			"<show>away</show>"
		"</presence>";
	m_buddy1->setStatusMessage("");
	tag = m_buddy1->generatePresenceStanza(0);
	compare(tag, r);
}


void RosterManagerTest::sendUnavailablePresenceToAll() {
	m_manager->sendUnavailablePresenceToAll();
	testTagCount(0);
	
	std::string user1;
	std::string user2;
	user1 =	"<presence type='unavailable' to='user@example.com' from='user1%example.com@icq.localhost/bot' />";
	user2 =	"<presence type='unavailable' to='user@example.com' from='user2%example.com@icq.localhost/bot' />";
	
	setRoster();
	m_manager->sendUnavailablePresenceToAll();
	testTagCount(2);
	compare(user1);
	compare(user2);
	clearTags();
	
	m_buddy1->setOffline();
	m_buddy2->setOnline();
	m_manager->sendUnavailablePresenceToAll();
	testTagCount(1);
	compare(user2);
}

void RosterManagerTest::sendPresenceToAll() {
	std::string user1;
	std::string user2;
	user1 =	"<presence to='user@example.com' from='user1%example.com@icq.localhost/bot'>"
				"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
				"<status>I&apos;m here</status>"
			"</presence>";
	user2 =	"<presence to='user@example.com' from='user2%example.com@icq.localhost/bot'>"
				"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
				"<status>I&apos;m here</status>"
			"</presence>";
	
	setRoster();
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_buddy2->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy2->setStatusMessage("I'm here");
	m_buddy2->setIconHash("somehash");
	m_manager->sendPresenceToAll("user@example.com");
	
	testTagCount(2);
	compare(user1);
	compare(user2);
}

void RosterManagerTest::isInRoster() {
	setRoster();
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	m_manager->removeFromLocalRoster("user1@example.com");
	CPPUNIT_ASSERT (!m_manager->isInRoster("user1@example.com", ""));
}

void RosterManagerTest::addRosterItem() {
	m_manager->addRosterItem(m_buddy1);
	m_manager->addRosterItem(m_buddy2);
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
}

void RosterManagerTest::sendPresence() {
	std::string user1;
	std::string user2;
	std::string user3;
	user1 =	"<presence to='user@example.com/resource' from='user1%example.com@icq.localhost/bot'>"
				"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
				"<status>I&apos;m here</status>"
			"</presence>";
	user2 =	"<presence type='unavailable' to='user@example.com/resource' from='user2%example.com@icq.localhost'/>";
// 				"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
// 			"</presence>";
	user3 =	"<presence type='unavailable' to='user@example.com/resource' from='user3%example.com@icq.localhost' />";
	
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_manager->addRosterItem(m_buddy1);
	m_manager->sendPresence(m_buddy1, "resource");
	m_manager->sendPresence("user2@example.com", "resource");
	
	testTagCount(2);
	compare(user1);
	compare(user2);
	clearTags();

	m_manager->sendPresence("user3@example.com", "resource");
	testTagCount(1);
	compare(user3);
	clearTags();

	m_manager->sendPresence("user1@example.com", "resource");
	testTagCount(1);
	compare(user1);
}

void RosterManagerTest::handleBuddySignedOn() {
	std::string r;
	r =	"<presence from='user1%example.com@icq.localhost/bot' to='user@example.com'>"
			"<status>I'm here</status>"
			"<c xmlns='http://jabber.org/protocol/caps' hash='sha-1' node='http://spectrum.im/transport' ver='123'/>"
		"</presence>";
	
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_buddy1->setOffline();
	m_manager->addRosterItem(m_buddy1);

	m_manager->handleBuddySignedOn(m_buddy1);
	testTagCount(1);
	compare(r);

	CPPUNIT_ASSERT (m_buddy1->isOnline());
}

void RosterManagerTest::handleBuddySignedOff() {
	std::string r;
	r =	"<presence from='user1%example.com@icq.localhost/bot' type='unavailable' to='user@example.com'/>";

	m_buddy1->setStatus(PURPLE_STATUS_OFFLINE);
	m_buddy1->setOnline();
	m_manager->addRosterItem(m_buddy1);
	m_manager->handleBuddySignedOff(m_buddy1);

	testTagCount(1);
	compare(r);
	
	CPPUNIT_ASSERT (!m_buddy1->isOnline());
}

void RosterManagerTest::handleBuddyCreatedRIE() {
	std::string r;
	r =	"<iq to='user@example.com/psi' type='set' id='id1' from='icq.localhost'>"
			"<x xmlns='http://jabber.org/protocol/rosterx'>"
				"<item action='add' jid='user1%example.com@icq.localhost' name='Frank'>"
					"<group>Buddies</group>"
				"</item>"
				"<item action='add' jid='user2%example.com@icq.localhost' name='Bob'>"
					"<group>Buddies</group>"
				"</item>"
			"</x>"
		"</iq>";

	m_manager->handleBuddyCreated(m_buddy1);
	testTagCount(0);
	m_manager->syncBuddiesCallback();
	testTagCount(0);

	m_manager->handleBuddyCreated(m_buddy2);

	m_manager->syncBuddiesCallback();
	testTagCount(0);
	
	m_manager->syncBuddiesCallback();
	testTagCount(1);
	compare(r);
	
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
	
}

void RosterManagerTest::handleBuddyCreatedSubscribe() {
	std::string user1;
	std::string user2;
	user1 =	"<presence type='subscribe' to='user@example.com' from='user1%example.com@icq.localhost'>"
				"<nick xmlns='http://jabber.org/protocol/nick'>Frank</nick>"
			"</presence>";
	user2 =	"<presence type='subscribe' to='user@example.com' from='user2%example.com@icq.localhost'>"
				"<nick xmlns='http://jabber.org/protocol/nick'>Bob</nick>"
			"</presence>";

	m_user->setResource("psi", 10, 0); // no features => it should send subscribe instead of RIE.
	m_manager->handleBuddyCreated(m_buddy1);
	testTagCount(0);
	m_manager->syncBuddiesCallback();
	testTagCount(0);

	m_manager->handleBuddyCreated(m_buddy2);
	m_manager->syncBuddiesCallback();
	testTagCount(0);
	m_manager->syncBuddiesCallback();
	testTagCount(2);
	compare(user1);
	compare(user2);
	
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
	
}

void RosterManagerTest::handleBuddyCreatedRemove() {
	std::string r;
	r =	"<iq to='user@example.com/psi' type='set' id='id1' from='icq.localhost'>"
			"<x xmlns='http://jabber.org/protocol/rosterx'>"
				"<item action='add' jid='user2%example.com@icq.localhost' name='Bob'>"
					"<group>Buddies</group>"
				"</item>"
			"</x>"
		"</iq>";
	m_manager->handleBuddyCreated(m_buddy1);
	testTagCount(0);
	m_manager->syncBuddiesCallback();
	testTagCount(0);

	m_manager->handleBuddyCreated(m_buddy2);
	m_manager->syncBuddiesCallback();
	testTagCount(0);
	
	m_manager->handleBuddyRemoved(m_buddy1);
	m_manager->syncBuddiesCallback();
	testTagCount(0);

	m_manager->syncBuddiesCallback();
	testTagCount(1);
	compare(r);
	
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
	
}

void RosterManagerTest::handleSubscriptionSubscribe() {
	Subscription s(Subscription::Subscribe, JID("user1%example.com@icq.localhost"));
}
