#include "capabilityhandlertest.h"
#include "transport.h"
#include "testingbackend.h"
#include "../capabilityhandler.h"
#include "../usermanager.h"

void CapabilityHandlerTest::setUp (void) {
	m_handler = new CapabilityHandler();
	m_user = new TestingUser("key", "user@example.com");
	Transport::instance()->userManager()->addUser(m_user);
}

void CapabilityHandlerTest::tearDown (void) {
	delete m_handler;
	Transport::instance()->userManager()->removeUser(m_user);
}

void CapabilityHandlerTest::handleDiscoInfo() {
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/bot");
	
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", "user@example.com/bot");
	iq->addAttribute("id", "disco1");
	iq->addAttribute("to", "icq.localhost");
	iq->addAttribute("type", "result");
	
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "http://jabber.org/protocol/disco#info");
	query->addAttribute("node", "http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=");
	
	Tag *identity = new Tag("identity");
	identity->addAttribute("category", "client");
	identity->addAttribute("name", "Exodus 0.9.1");
	identity->addAttribute("type", "pc");
	query->addChild(identity);
	
	Tag *feature;
	feature = new Tag("feature");
	feature->addAttribute("var", "http://jabber.org/protocol/caps");
	query->addChild(feature);
	
	feature = new Tag("feature");
	feature->addAttribute("var", "http://jabber.org/protocol/disco#info");
	query->addChild(feature);
	
	feature = new Tag("feature");
	feature->addAttribute("var", "http://jabber.org/protocol/disco#items");
	query->addChild(feature);
	
	iq->addChild(query);
	
	m_handler->handleDiscoInfo(JID("user@example.com/bot"), iq, context);
}
