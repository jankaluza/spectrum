#ifndef CONFIG_FILE_TEST_H
#define CONFIG_FILE_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"
#include "abstracttest.h"

using namespace std;

class ConfigFile;

class ConfigFileTest : public AbstractTest
{
	CPPUNIT_TEST_SUITE (ConfigFileTest);
	CPPUNIT_TEST (loadString);
	CPPUNIT_TEST (loadBoolean);
	CPPUNIT_TEST (loadInteger);
	CPPUNIT_TEST (getConfig);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void up (void);
		void down (void);

	protected:
		void loadString();
		void loadBoolean();
		void loadInteger();
		void getConfig();

	private:
		ConfigFile *m_file;
};

CPPUNIT_TEST_SUITE_REGISTRATION (ConfigFileTest);

#endif
