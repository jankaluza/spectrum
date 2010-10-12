#include "registerhandlertest.h"
#include "registerhandler.h"
#include "transport.h"
#include "testingbackend.h"

void RegisterHandlerTest::up (void) {
	m_handler = new GlooxRegisterHandler();
	m_backend = (TestingBackend *) Transport::instance()->sql();
}

void RegisterHandlerTest::down (void) {
	delete m_handler;
	m_backend->reset();
}

void RegisterHandlerTest::handleIqNoPermission() {
	std::string r;
	r =	"<iq id='reg1' type='error' from='icq.localhost' to='user@spectrum.im/psi'>"
			"<error code='400' type='modify'>"
				"<bad-request xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
			"</error>"
		"</iq>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("example.com");
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "get");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	iq->addChild(query);
	
	m_handler->handleIq(iq);
	
	testTagCount(1);
	compare(r);
}

void RegisterHandlerTest::handleIqGetNewUser() {
	std::string r;
	r =	"<iq id='reg1' type='result' to='user@spectrum.im/psi' from='icq.localhost'>"
			"<query xmlns='jabber:iq:register'>"
				"<instructions>Enter your screenname and password:</instructions>"
				"<username/>"
				"<password/>"
				"<x xmlns='jabber:x:data' type='form'>"
					"<title>Registration</title>"
					"<instructions>Enter your screenname and password:</instructions>"
					"<field type='hidden' var='FORM_TYPE'>"
						"<value>jabber:iq:register</value>"
					"</field>"
					"<field type='text-single' var='username' label='UIN'>"
						"<required/>"
					"</field>"
					"<field type='text-private' var='password' label='Password'/>"
					"<field type='list-single' var='language' label='Language'>"
						"<value>en</value>";
	std::map <std::string, std::string> languages = localization.getLanguages();
	for (std::map <std::string, std::string>::iterator it = languages.begin(); it != languages.end(); it++) {
		r +=			"<option label='" + (*it).second + "'><value>" + (*it).first + "</value></option>";
	}
	r +=			"</field>"
					"<field type='text-single' var='encoding' label='Encoding'>"
						"<value>windows-1250</value>"
					"</field>"
				"</x>"
			"</query>"
		"</iq>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "get");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	iq->addChild(query);
	
	m_handler->handleIq(iq);
	testTagCount(1);
	compare(r);
}

void RegisterHandlerTest::handleIqGetExistingUser() {
	std::string r;
	r =	"<iq id='reg1' type='result' to='user@spectrum.im/psi' from='icq.localhost'>"
			"<query xmlns='jabber:iq:register'>"
				"<instructions>Enter your screenname and password:</instructions>"
				"<registered/>"
				"<username>someuin</username>"
				"<password/>"
				"<x xmlns='jabber:x:data' type='form'>"
					"<title>Registration</title>"
					"<instructions>Enter your screenname and password:</instructions>"
					"<field type='hidden' var='FORM_TYPE'>"
						"<value>jabber:iq:register</value>"
					"</field>"
					"<field type='text-single' var='username' label='UIN'>"
						"<required/>"
						"<value>someuin</value>"
					"</field>"
					"<field type='text-private' var='password' label='Password'/>"
					"<field type='list-single' var='language' label='Language'>"
						"<value>cs</value>";
	std::map <std::string, std::string> languages = localization.getLanguages();
	for (std::map <std::string, std::string>::iterator it = languages.begin(); it != languages.end(); it++) {
		r +=			"<option label='" + (*it).second + "'><value>" + (*it).first + "</value></option>";
	}
	r +=			"</field>"
					"<field type='text-single' var='encoding' label='Encoding'>"
						"<value>utf8</value>"
					"</field>"
					"<field type='boolean' var='unregister' label='Remove your registration'>"
						"<value>0</value>"
					"</field>"
				"</x>"
			"</query>"
		"</iq>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	UserRow row;
	row.jid = "user@spectrum.im";
	row.uin = "someuin";
	row.password = "secret";
	row.language = "cs";
	row.encoding = "utf8";
	m_backend->addUser(row);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "get");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	iq->addChild(query);
	
	m_handler->handleIq(iq);
	testTagCount(1);
	compare(r);
}

void RegisterHandlerTest::handleIqRegisterLegacy() {
	std::string r;
	std::string presence;
	r =			"<iq id='reg1' type='result' from='icq.localhost' to='user@spectrum.im/psi'/>";
	presence =	"<presence type='subscribe' from='icq.localhost' to='user@spectrum.im' from='icq.localhost'/>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "set");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	query->addChild( new Tag("username", "someuin") );
	query->addChild( new Tag("password", "secret") );
	iq->addChild(query);

	UserRow row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id == -1);
	
	m_handler->handleIq(iq);
	testTagCount(2);
	compare(r);
	compare(presence);
	
	row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id != -1);
}

void RegisterHandlerTest::handleIqRegisterLegacyNoPassword() {
	std::string r;
	r =	"<iq id='reg1' type='error' from='icq.localhost' to='user@spectrum.im/psi'>"
			"<error code='406' type='modify'>"
				"<not-acceptable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
			"</error>"
		"</iq>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "set");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	query->addChild( new Tag("username", "someuin") );
	query->addChild( new Tag("password", "") );
	iq->addChild(query);
	
	m_handler->handleIq(iq);
	testTagCount(1);
	compare(r);
}

void RegisterHandlerTest::handleIqRegisterLegacyNoUsername() {
	std::string r;
	r =	"<iq id='reg1' type='error' from='icq.localhost' to='user@spectrum.im/psi'>"
			"<error code='406' type='modify'>"
				"<not-acceptable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
			"</error>"
		"</iq>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "set");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	query->addChild( new Tag("username", "") );
	query->addChild( new Tag("password", "secret") );
	iq->addChild(query);
	
	m_handler->handleIq(iq);
	testTagCount(1);
	compare(r);
}

void RegisterHandlerTest::handleIqRegisterXData() {
	std::string r;
	std::string presence;
	r =			"<iq id='reg1' type='result' from='icq.localhost' to='user@spectrum.im/psi'/>";
	presence =	"<presence type='subscribe' from='icq.localhost' to='user@spectrum.im' from='icq.localhost'/>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "set");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	Tag *x = new Tag("x");
	x->addAttribute("xmlns", "jabber:x:data");
	x->addAttribute("type", "submit");

	Tag *field = new Tag("field");
	field->addAttribute("type", "hidden");
	field->addAttribute("var", "FORM_TYPE");
	field->addChild( new Tag("value", "jabber:iq:register") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "text-single");
	field->addAttribute("var", "username");
	field->addAttribute("label", "Network username");
	field->addChild( new Tag("required") );
	field->addChild( new Tag("value", "someuin") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "text-private");
	field->addAttribute("var", "password");
	field->addAttribute("label", "Password");
	field->addChild( new Tag("value", "secret") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "list-single");
	field->addAttribute("var", "language");
	field->addAttribute("label", "Language");
	field->addChild( new Tag("value", "en") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "text-single");
	field->addAttribute("var", "encoding");
	field->addAttribute("label", "Encoding");
	field->addChild( new Tag("value", "windows-1250") );
	x->addChild(field);
	query->addChild(x);
	iq->addChild(query);

	UserRow row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id == -1);
	
	m_handler->handleIq(iq);
	testTagCount(2);
	compare(r);
	compare(presence);
	
	row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id != -1);
}


void RegisterHandlerTest::handleIqRegisterXDataBadLanguage() {
	std::string r;
	r =	"<iq id='reg1' type='error' from='icq.localhost' to='user@spectrum.im/psi'>"
			"<error code='406' type='modify'>"
				"<not-acceptable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
			"</error>"
		"</iq>";

	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	cfg.enable_public_registration = 1;
	m_backend->setConfiguration(cfg);
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "set");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	Tag *x = new Tag("x");
	x->addAttribute("xmlns", "jabber:x:data");
	x->addAttribute("type", "submit");

	Tag *field = new Tag("field");
	field->addAttribute("type", "hidden");
	field->addAttribute("var", "FORM_TYPE");
	field->addChild( new Tag("value", "jabber:iq:register") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "text-single");
	field->addAttribute("var", "username");
	field->addAttribute("label", "Network username");
	field->addChild( new Tag("required") );
	field->addChild( new Tag("value", "someuin") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "text-private");
	field->addAttribute("var", "password");
	field->addAttribute("label", "Password");
	field->addChild( new Tag("value", "secret") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "list-single");
	field->addAttribute("var", "language");
	field->addAttribute("label", "Language");
	field->addChild( new Tag("value", "unknown_language") );
	x->addChild(field);

	field = new Tag("field");
	field->addAttribute("type", "text-single");
	field->addAttribute("var", "encoding");
	field->addAttribute("label", "Encoding");
	field->addChild( new Tag("value", "windows-1250") );
	x->addChild(field);
	query->addChild(x);
	iq->addChild(query);

	UserRow row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id == -1);
	
	m_handler->handleIq(iq);
	testTagCount(1);
	compare(r);
	
	row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id == -1);
}
