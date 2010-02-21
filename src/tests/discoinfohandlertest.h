#ifndef GLOOX_DISCO_INFO_HANDLER_TEST_H
#define GLOOX_DISCO_INFO_HANDLER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "gloox/tag.h"
#include "abstracttest.h"

using namespace gloox;

class GlooxDiscoInfoHandler;

class GlooxDiscoInfoHandlerTest : public AbstractTest
{
	CPPUNIT_TEST_SUITE (GlooxDiscoInfoHandlerTest);
	CPPUNIT_TEST (handleIq);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);
		Tag *getCorrectTag(const std::string &to);

	protected:
		void handleIq();

	private:
		GlooxDiscoInfoHandler *m_handler;
};

CPPUNIT_TEST_SUITE_REGISTRATION (GlooxDiscoInfoHandlerTest);

#endif
