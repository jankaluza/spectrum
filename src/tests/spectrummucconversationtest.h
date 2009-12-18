#ifndef SPECTRUM_MUCCONVERSATION_TEST_H
#define SPECTRUM_MUCCONVERSATION_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "testinguser.h"

using namespace std;

class SpectrumMUCConversation;

class SpectrumMUCConversationTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (SpectrumMUCConversationTest);
	CPPUNIT_TEST (handleMessage);
	CPPUNIT_TEST (addUsers);
	CPPUNIT_TEST (renameUser);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void handleMessage();
		void addUsers();
		void renameUser();
		
	private:
		SpectrumMUCConversation *m_conv;
		TestingUser *m_user;
		std::list <Tag *> m_tags;
};

CPPUNIT_TEST_SUITE_REGISTRATION (SpectrumMUCConversationTest);

#endif
