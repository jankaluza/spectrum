#ifndef AUTOCONNECT_LOOP_TEST_H
#define AUTOCONNECT_LOOP_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "gloox/tag.h"
#include "abstracttest.h"

using namespace gloox;

class AutoConnectLoop;

class AutoConnectLoopTest : public AbstractTest
{
	CPPUNIT_TEST_SUITE (AutoConnectLoopTest);
	CPPUNIT_TEST (restoreNextConnection);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);

	protected:
		void restoreNextConnection();

	private:
		AutoConnectLoop *m_loop;
};

CPPUNIT_TEST_SUITE_REGISTRATION (AutoConnectLoopTest);

#endif
