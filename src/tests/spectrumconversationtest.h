#ifndef SPECTRUM_CONVERSATION_TEST_H
#define SPECTRUM_CONVERSATION_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "testinguser.h"

using namespace std;

class SpectrumConversation;

class SpectrumConversationTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (SpectrumConversationTest);
	CPPUNIT_TEST (handleMessage);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void handleMessage();
		
	private:
		SpectrumConversation *m_conv;
		TestingUser *m_user;
		std::list <Tag *> m_tags;
};

CPPUNIT_TEST_SUITE_REGISTRATION (SpectrumConversationTest);

#endif
