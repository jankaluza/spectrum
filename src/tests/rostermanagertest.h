// CppUnit-Tutorial
// file: fractiontest.h
#ifndef ROSTER_MANAGER_TEST_H
#define ROSTER_MANAGER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "spectrumbuddytest.h"
#include "testinguser.h"

using namespace std;

class SpectrumRosterManager;

class RosterManagerTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (RosterManagerTest);
	CPPUNIT_TEST (setRoster);
	CPPUNIT_TEST (generatePresenceStanza);
	CPPUNIT_TEST (sendUnavailablePresenceToAll);
	CPPUNIT_TEST (sendPresenceToAll);
	CPPUNIT_TEST (isInRoster);
	CPPUNIT_TEST (addRosterItem);
	CPPUNIT_TEST (sendPresence);
	CPPUNIT_TEST (handleBuddySignedOn);
	CPPUNIT_TEST (handleBuddySignedOff);
	CPPUNIT_TEST (handleBuddyCreatedRIE);
	CPPUNIT_TEST (handleBuddyCreatedSubscribe);
	CPPUNIT_TEST (handleBuddyCreatedRemove);
	CPPUNIT_TEST (handleSubscriptionSubscribe);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void setRoster();
		void generatePresenceStanza();
		void sendUnavailablePresenceToAll();
		void sendPresenceToAll();
		void isInRoster();
		void addRosterItem();
		void sendPresence();
		void handleBuddySignedOn();
		void handleBuddySignedOff();
		void handleBuddyCreatedRIE();
		void handleBuddyCreatedSubscribe();
		void handleBuddyCreatedRemove();
		void handleSubscriptionSubscribe();
		
	private:
		SpectrumBuddyTest *m_buddy1;
		SpectrumBuddyTest *m_buddy2;
		SpectrumRosterManager *m_manager;
		TestingUser *m_user;
		std::list <Tag *> m_tags;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RosterManagerTest);

#endif
