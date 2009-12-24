#ifndef AUTOCONNECT_LOOP_TEST_H
#define AUTOCONNECT_LOOP_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "gloox/tag.h"

using namespace gloox;

class AutoConnectLoop;

class AutoConnectLoopTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (AutoConnectLoopTest);
	CPPUNIT_TEST (restoreNextConnection);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void restoreNextConnection();

	private:
		AutoConnectLoop *m_loop;
		std::list <Tag *> m_tags;
};

CPPUNIT_TEST_SUITE_REGISTRATION (AutoConnectLoopTest);

#endif
