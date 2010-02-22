#ifndef ROSTER_MANAGER_TEST_H
#define ROSTER_MANAGER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "spectrumbuddytest.h"
#include "testinguser.h"
#include "abstracttest.h"

using namespace std;

class SpectrumRosterManager;

class RosterManagerTest : public AbstractTest {
	CPPUNIT_TEST_SUITE (RosterManagerTest);
	CPPUNIT_TEST (setRoster);
	CPPUNIT_TEST (generatePresenceStanza);
	CPPUNIT_TEST (sendUnavailablePresenceToAll);
	CPPUNIT_TEST (sendPresenceToAll);
	CPPUNIT_TEST (isInRoster);
	CPPUNIT_TEST (addRosterItem);
	CPPUNIT_TEST (sendPresence);
	CPPUNIT_TEST (handleBuddySignedOn);
	CPPUNIT_TEST (handleBuddyStatusChanged);
	CPPUNIT_TEST (handleBuddySignedOff);
	CPPUNIT_TEST (handleBuddyCreatedRIE);
	CPPUNIT_TEST (handleBuddyCreatedRIEOneBoth);
	CPPUNIT_TEST (handleBuddyCreatedSubscribe);
	CPPUNIT_TEST (handleBuddyCreatedSubscribeOneBoth);
	CPPUNIT_TEST (handleBuddyCreatedRemove);
	CPPUNIT_TEST (handlePresenceProbe);
	CPPUNIT_TEST (handleAuthorizationRequest);
	CPPUNIT_TEST (handleSubscriptionSubscribe);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);

	protected:
		void setRoster();
		void generatePresenceStanza();
		void sendUnavailablePresenceToAll();
		void sendPresenceToAll();
		void isInRoster();
		void addRosterItem();
		void sendPresence();
		void handleBuddySignedOn();
		void handleBuddyStatusChanged();
		void handleBuddySignedOff();
		void handleBuddyCreatedRIE();
		void handleBuddyCreatedRIEOneBoth();
		void handleBuddyCreatedSubscribe();
		void handleBuddyCreatedSubscribeOneBoth();
		void handleBuddyCreatedRemove();
		void handlePresenceProbe();
		void handleAuthorizationRequest();
		void handleSubscriptionSubscribe();
		
	private:
		SpectrumBuddyTest *m_buddy1;
		SpectrumBuddyTest *m_buddy2;
		SpectrumRosterManager *m_manager;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RosterManagerTest);

#endif
