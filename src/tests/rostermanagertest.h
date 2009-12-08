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

class RosterManager;

class RosterManagerTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (RosterManagerTest);
	CPPUNIT_TEST (setRoster);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void setRoster (void);
		
	private:
		SpectrumBuddyTest *m_buddy1;
		SpectrumBuddyTest *m_buddy2;
		RosterManager *m_manager;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RosterManagerTest);

#endif
