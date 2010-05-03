#include "configfiletest.h"
#include "configfile.h"
#include "../capabilityhandler.h"
#include "transport.h"
#include "log.h"

void ConfigFileTest::up (void) {
	m_file = new ConfigFile("");
}

void ConfigFileTest::down (void) {
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

void ConfigFileTest::getConfig() {
	std::string config = ""
	"[service]\n"
	"enable=1\n"
	"server=127.0.0.1\n"
	"use_proxy=0\n"
	"jid=icq.localhost\n"
	"password=secret\n"
	"port=5347\n"
	"admins=admin@example.com;foo@bar.cz\n"
	"filetransfer_cache=/var/lib/spectrum/filetransfer_cache\n"
	"protocol=icq\n"
	"name=My ICQ Transport\n"
	"language=en\n"
	"transport_features = avatars;chatstate;filetransfer\n"
	"vip_mode=1\n"
	"only_for_vip=1\n"
	"allowed_servers=localhost\n"
	"vip_features = avatars;chatstate;filetransfer\n"
	"pid_file=/var/run/spectrum/$jid\n"
	"\n"
	"[logging]\n"
	"log_file=/var/log/spectrum/$jid.log\n"
	"log_areas=xml;purple\n"
	"\n"
	"[database]\n"
	"type=mysql\n"
	"host=localhost\n"
	"user=user\n"
	"password=password\n"
	"database=/var/lib/spectrum/$jid/spectrum.db\n"
	"prefix=icq_\n"
	"\n"
	"[purple]\n"
	"bind=0.0.0.0\n"
	"userdir=/var/lib/spectrum/$jid\n";
	m_file->loadFromData(config);
	CPPUNIT_ASSERT (m_file->m_loaded);
	
	Configuration conf = m_file->getConfiguration();
	CPPUNIT_ASSERT (conf.discoName == "My ICQ Transport");
	CPPUNIT_ASSERT (conf.protocol == "icq");
	CPPUNIT_ASSERT (conf.server == "127.0.0.1");
	CPPUNIT_ASSERT (conf.password == "secret");
	CPPUNIT_ASSERT (conf.jid == "icq.localhost");
	CPPUNIT_ASSERT (conf.port == 5347);
	CPPUNIT_ASSERT (conf.logAreas == (LOG_AREA_XML | LOG_AREA_PURPLE));
	CPPUNIT_ASSERT (conf.logfile == "/var/log/spectrum/icq.localhost.log");
	CPPUNIT_ASSERT (conf.pid_f == "/var/run/spectrum/icq.localhost");
	CPPUNIT_ASSERT (conf.onlyForVIP == true);
	CPPUNIT_ASSERT (conf.VIPEnabled == true);
	CPPUNIT_ASSERT (conf.transportFeatures == (TRANSPORT_FEATURE_AVATARS | TRANSPORT_FEATURE_FILETRANSFER | TRANSPORT_FEATURE_TYPING_NOTIFY));
	CPPUNIT_ASSERT (conf.language == "en");
	CPPUNIT_ASSERT (conf.VIPFeatures == (TRANSPORT_FEATURE_AVATARS | TRANSPORT_FEATURE_FILETRANSFER | TRANSPORT_FEATURE_TYPING_NOTIFY));
	CPPUNIT_ASSERT (conf.useProxy == false);
	CPPUNIT_ASSERT (conf.allowedServers.front() == "localhost");
	CPPUNIT_ASSERT (conf.admins.front() == "admin@example.com");
	CPPUNIT_ASSERT (conf.admins.back() == "foo@bar.cz");
// 	CPPUNIT_ASSERT (conf.bindIPs[0] == "0.0.0.0");
	CPPUNIT_ASSERT (conf.userDir == "/var/lib/spectrum/icq.localhost");
	CPPUNIT_ASSERT (conf.filetransferCache == "/var/lib/spectrum/filetransfer_cache");
	CPPUNIT_ASSERT (conf.sqlHost == "localhost");
	CPPUNIT_ASSERT (conf.sqlPassword == "password");
	CPPUNIT_ASSERT (conf.sqlUser == "user");
	CPPUNIT_ASSERT (conf.sqlDb == "/var/lib/spectrum/icq.localhost/spectrum.db");
	CPPUNIT_ASSERT (conf.sqlPrefix == "icq_");
	CPPUNIT_ASSERT (conf.sqlType == "mysql");
}
