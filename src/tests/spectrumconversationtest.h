#ifndef SPECTRUM_CONVERSATION_TEST_H
#define SPECTRUM_CONVERSATION_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "testinguser.h"
#include "abstracttest.h"

using namespace std;

class SpectrumConversation;

class SpectrumConversationTest : public AbstractTest {
	CPPUNIT_TEST_SUITE (SpectrumConversationTest);
	CPPUNIT_TEST (handleMessage);
	CPPUNIT_TEST (handleMessageDelay);
// 	CPPUNIT_TEST (handleMessageMSNTimeoutError);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);

	protected:
		void handleMessage();
		void handleMessageDelay();
		void handleMessageMSNTimeoutError();
		
	private:
		SpectrumConversation *m_conv;
		TestingUser *m_user;
};

CPPUNIT_TEST_SUITE_REGISTRATION (SpectrumConversationTest);

#endif
