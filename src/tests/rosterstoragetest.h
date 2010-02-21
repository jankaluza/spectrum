#ifndef ROSTER_STORAGE_TEST_H
#define ROSTER_STORAGE_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "spectrumbuddytest.h"
#include "testinguser.h"
#include "abstracttest.h"

using namespace std;

class RosterStorage;

class RosterStorageTest : public AbstractTest
{
	CPPUNIT_TEST_SUITE (RosterStorageTest);
	CPPUNIT_TEST (storeBuddies);
	CPPUNIT_TEST (storeBuddiesRemove);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);

	protected:
		void storeBuddies();
		void storeBuddiesRemove();

	private:
		SpectrumBuddyTest *m_buddy1;
		SpectrumBuddyTest *m_buddy2;
		RosterStorage *m_storage;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RosterStorageTest);

#endif
