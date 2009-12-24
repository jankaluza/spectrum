#ifndef GLOOX_DISCO_INFO_HANDLER_TEST_H
#define GLOOX_DISCO_INFO_HANDLER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "gloox/tag.h"

using namespace gloox;

class GlooxDiscoInfoHandler;

class GlooxDiscoInfoHandlerTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (GlooxDiscoInfoHandlerTest);
	CPPUNIT_TEST (handleIq);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);
		Tag *getCorrectTag(const std::string &to);

	protected:
		void handleIq();

	private:
		GlooxDiscoInfoHandler *m_handler;
		std::list <Tag *> m_tags;
};

CPPUNIT_TEST_SUITE_REGISTRATION (GlooxDiscoInfoHandlerTest);

#endif
