#include "registerhandlertest.h"
#include "registerhandler.h"
#include "transport.h"
#include "testingbackend.h"

void RegisterHandlerTest::setUp (void) {
	m_handler = new GlooxRegisterHandler();
	m_backend = (TestingBackend *) Transport::instance()->sql();
}

void RegisterHandlerTest::tearDown (void) {
	delete m_handler;
	m_backend->reset();
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		m_tags.remove(tag);
		delete tag;
	}
	Transport::instance()->clearTags();
}

void RegisterHandlerTest::handleIqNoPermission() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("example.com");
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
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 1);
	
	Tag *tag = m_tags.front();
	
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "error");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findChild("error") != NULL);
	CPPUNIT_ASSERT (tag->findChild("error")->findAttribute("code") == "400");
	CPPUNIT_ASSERT (tag->findChild("error")->findAttribute("type") == "modify");
	CPPUNIT_ASSERT (tag->findChild("error")->findChild("bad-request") != NULL);
	CPPUNIT_ASSERT (tag->findChild("error")->findChild("bad-request")->findAttribute("xmlns") == "urn:ietf:params:xml:ns:xmpp-stanzas");
}

void RegisterHandlerTest::handleIqGetNewUser() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
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
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 1);
	
	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "result");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	CPPUNIT_ASSERT (tag->findChild("query", "xmlns", "jabber:iq:register") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("instructions") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("instructions")->cdata() != "");
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("registered") == NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("username") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("username")->cdata() == "");
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("password") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("password")->cdata() == "");
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("x") != NULL);
	Tag *x = tag->findChild("query")->findChild("x");
	CPPUNIT_ASSERT (x->findAttribute("xmlns") == "jabber:x:data");
	CPPUNIT_ASSERT (x->findAttribute("type") == "form");
	CPPUNIT_ASSERT (x->findChild("title") != NULL);
	CPPUNIT_ASSERT (x->findChild("title")->cdata() != "");
	CPPUNIT_ASSERT (x->findChild("instructions") != NULL);
	CPPUNIT_ASSERT (x->findChild("instructions")->cdata() != "");
	CPPUNIT_ASSERT (x->findChild("instructions")->cdata() != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE")->findAttribute("type") == "hidden");
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE")->findChild("value")->cdata() == "jabber:iq:register");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findAttribute("type") == "text-single");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findChild("value") == NULL);
// 	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findChild("value")->cdata() == "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findChild("required") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "password") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "password")->findAttribute("type") == "text-private");
	CPPUNIT_ASSERT (x->findChild("field", "var", "password")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "password")->findChild("value") == NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findAttribute("type") == "list-single");
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("value")->cdata() == "en");
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("required") == NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("option") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findAttribute("type") == "text-single");
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findChild("value")->cdata() == "windows-1250");
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findChild("required") == NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister") == NULL);
// 	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findAttribute("type") == "boolean");
// 	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findAttribute("label") != "");
// 	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findChild("value") != NULL);
// 	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findChild("value")->cdata() == "0");
}

void RegisterHandlerTest::handleIqGetExistingUser() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
	m_backend->setConfiguration(cfg);
	
	m_backend->addUser("user@spectrum.im", "someuin", "secret", "cs", "utf8");
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@spectrum.im/psi");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "get");
	iq->addAttribute("id", "reg1");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:register");
	iq->addChild(query);
	
	m_handler->handleIq(iq);
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 1);
	
	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "result");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	CPPUNIT_ASSERT (tag->findChild("query", "xmlns", "jabber:iq:register") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("instructions") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("instructions")->cdata() != "");
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("registered") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("username") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("username")->cdata() == "someuin");
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("password") != NULL);
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("password")->cdata() == "");
	CPPUNIT_ASSERT (tag->findChild("query")->findChild("x") != NULL);
	Tag *x = tag->findChild("query")->findChild("x");
	CPPUNIT_ASSERT (x->findAttribute("xmlns") == "jabber:x:data");
	CPPUNIT_ASSERT (x->findAttribute("type") == "form");
	CPPUNIT_ASSERT (x->findChild("title") != NULL);
	CPPUNIT_ASSERT (x->findChild("title")->cdata() != "");
	CPPUNIT_ASSERT (x->findChild("instructions") != NULL);
	CPPUNIT_ASSERT (x->findChild("instructions")->cdata() != "");
	CPPUNIT_ASSERT (x->findChild("instructions")->cdata() != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE")->findAttribute("type") == "hidden");
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "FORM_TYPE")->findChild("value")->cdata() == "jabber:iq:register");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findAttribute("type") == "text-single");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findChild("value")->cdata() == "someuin");
	CPPUNIT_ASSERT (x->findChild("field", "var", "username")->findChild("required") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "password") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "password")->findAttribute("type") == "text-private");
	CPPUNIT_ASSERT (x->findChild("field", "var", "password")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "password")->findChild("value") == NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findAttribute("type") == "list-single");
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("value")->cdata() == "cs");
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("required") == NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "language")->findChild("option") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findAttribute("type") == "text-single");
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findChild("value")->cdata() == "utf8");
	CPPUNIT_ASSERT (x->findChild("field", "var", "encoding")->findChild("required") == NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findAttribute("type") == "boolean");
	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findAttribute("label") != "");
	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findChild("value") != NULL);
	CPPUNIT_ASSERT (x->findChild("field", "var", "unregister")->findChild("value")->cdata() == "0");
}

void RegisterHandlerTest::handleIqRegisterLegacy() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
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
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 2);
	
	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "result");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	
	tag = m_tags.back();
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "subscribe");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	
	row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id != -1);
}

void RegisterHandlerTest::handleIqRegisterLegacyNoPassword() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
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

	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 1);

	Tag *tag = m_tags.front();
	
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "error");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findChild("error") != NULL);
	CPPUNIT_ASSERT (tag->findChild("error")->findAttribute("code") == "406");
	CPPUNIT_ASSERT (tag->findChild("error")->findAttribute("type") == "modify");
	CPPUNIT_ASSERT (tag->findChild("error")->findChild("not-acceptable") != NULL);
	CPPUNIT_ASSERT (tag->findChild("error")->findChild("not-acceptable")->findAttribute("xmlns") == "urn:ietf:params:xml:ns:xmpp-stanzas");
}

void RegisterHandlerTest::handleIqRegisterLegacyNoUsername() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
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

	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 1);

	Tag *tag = m_tags.front();
	
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "error");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findChild("error") != NULL);
	CPPUNIT_ASSERT (tag->findChild("error")->findAttribute("code") == "406");
	CPPUNIT_ASSERT (tag->findChild("error")->findAttribute("type") == "modify");
	CPPUNIT_ASSERT (tag->findChild("error")->findChild("not-acceptable") != NULL);
	CPPUNIT_ASSERT (tag->findChild("error")->findChild("not-acceptable")->findAttribute("xmlns") == "urn:ietf:params:xml:ns:xmpp-stanzas");
}

void RegisterHandlerTest::handleIqRegisterXData() {
	Configuration cfg;
	cfg.onlyForVIP = true;
	cfg.allowedServers.push_back("spectrum.im");
	cfg.language = "en";
	cfg.encoding = "windows-1250";
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
	field->addChild( new Tag("value", "cs") );
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
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT (m_tags.size() == 2);
	
	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "result");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im/psi");
	CPPUNIT_ASSERT (tag->findAttribute("id") == "reg1");
	
	tag = m_tags.back();
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "subscribe");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@spectrum.im");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	
	row = m_backend->getUserByJid("user@spectrum.im");
	CPPUNIT_ASSERT (row.id != -1);
}
