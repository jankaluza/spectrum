#ifndef REGISTER_HANDLER_TEST_H
#define REGISTER_HANDLER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "spectrumbuddytest.h"
#include "testinguser.h"

using namespace std;

class GlooxRegisterHandler;
class TestingBackend;

class RegisterHandlerTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (RegisterHandlerTest);
	CPPUNIT_TEST (handleIqNoPermission);
	CPPUNIT_TEST (handleIqGetNewUser);
	CPPUNIT_TEST (handleIqGetExistingUser);
	CPPUNIT_TEST (handleIqRegisterLegacy);
	CPPUNIT_TEST (handleIqRegisterLegacyNoPassword);
	CPPUNIT_TEST (handleIqRegisterXData);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

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
		std::list <Tag *> m_tags;
};

CPPUNIT_TEST_SUITE_REGISTRATION (RegisterHandlerTest);

#endif
