#ifndef ROSTER_STORAGE_TEST_H
#define ROSTER_STORAGE_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "spectrumbuddytest.h"
#include "testinguser.h"

using namespace std;

class RosterStorage;

class RosterStorageTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (RosterStorageTest);
	CPPUNIT_TEST (addRoster);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void addRoster();

	private:
		SpectrumBuddyTest *m_buddy1;
		SpectrumBuddyTest *m_buddy2;
		RosterStorage *m_storage;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RosterStorageTest);

#endif
