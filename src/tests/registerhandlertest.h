#ifndef REGISTER_HANDLER_TEST_H
#define REGISTER_HANDLER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "spectrumbuddytest.h"
#include "testinguser.h"
#include "abstracttest.h"

using namespace std;

class GlooxRegisterHandler;
class TestingBackend;

class RegisterHandlerTest : public AbstractTest
{
	CPPUNIT_TEST_SUITE (RegisterHandlerTest);
	// Everybody can register now...
// 	CPPUNIT_TEST (handleIqNoPermission);
	CPPUNIT_TEST (handleIqGetNewUser);
	CPPUNIT_TEST (handleIqGetExistingUser);
	CPPUNIT_TEST (handleIqRegisterLegacy);
	CPPUNIT_TEST (handleIqRegisterLegacyNoPassword);
	CPPUNIT_TEST (handleIqRegisterXData);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);

	protected:
		void handleIqNoPermission();
		void handleIqGetNewUser();
		void handleIqGetExistingUser();
		void handleIqRegisterLegacy();
		void handleIqRegisterLegacyNoPassword();
		void handleIqRegisterLegacyNoUsername();
		void handleIqRegisterXData();

	private:
		GlooxRegisterHandler *m_handler;
		TestingUser *m_user;
		TestingBackend *m_backend;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RegisterHandlerTest);

#endif
