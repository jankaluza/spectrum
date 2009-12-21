#include "configfiletest.h"
#include "configfile.h"
#include "../caps.h"

void ConfigFileTest::setUp (void) {
	m_file = new ConfigFile("");
}

void ConfigFileTest::tearDown (void) {
	delete m_file;
}

void ConfigFileTest::loadString() {
	m_file->m_jid = "user@server.com";
	m_file->m_protocol = "msn";
	std::string value;
	std::string config = 	"# comment on first line\n"
							"[section]\n"
							"variable = $jid-$protocol.something\n"
							"variable2 = value with space\n"
							"variable3 = $protocol\n";
	m_file->loadFromData(config);
	CPPUNIT_ASSERT (m_file->m_loaded);
	
	CPPUNIT_ASSERT (m_file->loadString(value, "section", "variable") == true);
	CPPUNIT_ASSERT (value == "user@server.com-msn.something");
	CPPUNIT_ASSERT (m_file->loadString(value, "section", "variable2") == true);
	CPPUNIT_ASSERT (value == "value with space");
	CPPUNIT_ASSERT (m_file->loadString(value, "section", "variable3") == true);
	CPPUNIT_ASSERT (value == "msn");
	CPPUNIT_ASSERT (m_file->loadString(value, "section", "variable4", "default value") == true);
	CPPUNIT_ASSERT (value == "default value");
	CPPUNIT_ASSERT (m_file->loadString(value, "section", "variable4") == false);
}


void ConfigFileTest::loadBoolean() {
	m_file->m_jid = "user@server.com";
	m_file->m_protocol = "msn";
	bool value;
	std::string config = 	"# comment on first line\n"
							"[section]\n"
							"variable = true\n"
							"variable2 = 1\n"
							"variable3 = 0\n"
							"variable4 = false\n"
							"variable5 = something\n";
	m_file->loadFromData(config);
	CPPUNIT_ASSERT (m_file->m_loaded);
	
	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable") == true);
	CPPUNIT_ASSERT (value == true);
	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable2") == true);
	CPPUNIT_ASSERT (value == true);
	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable3") == true);
	CPPUNIT_ASSERT (value == false);
	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable4") == true);
	CPPUNIT_ASSERT (value == false);

	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable5") == true);
	CPPUNIT_ASSERT (value == false);
	
	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable6", true) == true);
	CPPUNIT_ASSERT (value == true);
	
	CPPUNIT_ASSERT (m_file->loadBoolean(value, "section", "variable6", true, true) == false);
}


void ConfigFileTest::loadInteger() {
	m_file->m_jid = "user@server.com";
	m_file->m_protocol = "msn";
	int value;
	std::string config = 	"# comment on first line\n"
							"[section]\n"
							"variable = -50\n"
							"variable2 = 12\n"
							"variable3 = 3333333333333333333\n";
	m_file->loadFromData(config);
	CPPUNIT_ASSERT (m_file->m_loaded);
	
	CPPUNIT_ASSERT (m_file->loadInteger(value, "section", "variable") == true);
	CPPUNIT_ASSERT (value == -50);
	CPPUNIT_ASSERT (m_file->loadInteger(value, "section", "variable2") == true);
	CPPUNIT_ASSERT (value == 12);
	CPPUNIT_ASSERT (m_file->loadInteger(value, "section", "variable3") == false);
	CPPUNIT_ASSERT (m_file->loadInteger(value, "section", "variable4") == false);
	CPPUNIT_ASSERT (m_file->loadInteger(value, "section", "variable4", 1) == true);
	CPPUNIT_ASSERT (value == 1);
}
