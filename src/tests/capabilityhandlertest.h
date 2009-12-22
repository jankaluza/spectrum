#ifndef CAPABILITY_HANDLER_TEST_H
#define CAPABILITY_HANDLER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "testinguser.h"

using namespace std;

class CapabilityHandler;

class CapabilityHandlerTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (CapabilityHandlerTest);
	CPPUNIT_TEST (handleDiscoInfo);
	CPPUNIT_TEST (handleDiscoInfoNoWait);
	CPPUNIT_TEST (handleDiscoInfoCapsSet);
	CPPUNIT_TEST (handleDiscoInfoNotReady);
	CPPUNIT_TEST (handleDiscoInfoNoUser);
	CPPUNIT_TEST (handleDiscoInfoBadIdentity);
	CPPUNIT_TEST (handleDiscoInfoNoIdentity);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);
		Tag *getCorrectQuery();

	protected:
		void handleDiscoInfo();
		void handleDiscoInfoNoWait();
		void handleDiscoInfoCapsSet();
		void handleDiscoInfoNotReady();
		void handleDiscoInfoNoUser();
		void handleDiscoInfoBadIdentity();
		void handleDiscoInfoNoIdentity();

	private:
		CapabilityHandler *m_handler;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (CapabilityHandlerTest);

#endif
