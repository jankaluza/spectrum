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
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void handleDiscoInfo();

	private:
		CapabilityHandler *m_handler;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (CapabilityHandlerTest);

#endif
