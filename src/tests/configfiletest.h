#ifndef CONFIG_FILE_TEST_H
#define CONFIG_FILE_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"

using namespace std;

class ConfigFile;

class ConfigFileTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (ConfigFileTest);
	CPPUNIT_TEST (loadString);
	CPPUNIT_TEST (loadBoolean);
	CPPUNIT_TEST (loadInteger);
	CPPUNIT_TEST (getConfig);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

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
