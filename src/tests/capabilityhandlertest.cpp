#include "capabilityhandlertest.h"
#include "transport.h"
#include "testingbackend.h"
#include "../capabilityhandler.h"
#include "../usermanager.h"

void CapabilityHandlerTest::setUp (void) {
	m_handler = new CapabilityHandler();
	m_user = new TestingUser("user@example.com", "user@example.com");
	// Set resource without capabilities.
	m_user->removeResource("psi");
	m_user->setResource("psi", 50);
	Transport::instance()->userManager()->addUser(m_user);
}

void CapabilityHandlerTest::tearDown (void) {
	delete m_handler;
	Transport::instance()->userManager()->removeUser(m_user);
}

Tag *CapabilityHandlerTest::getCorrectQuery() {
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
	
	feature = new Tag("feature");
	feature->addAttribute("var", "http://jabber.org/protocol/xhtml-im");
	query->addChild(feature);
	
	feature = new Tag("feature");
	feature->addAttribute("var", "http://jabber.org/protocol/si/profile/file-transfer");
	query->addChild(feature);
	
	feature = new Tag("feature");
	feature->addAttribute("var", "http://jabber.org/protocol/chatstates");
	query->addChild(feature);
	return query;
}

void CapabilityHandlerTest::handleDiscoInfo() {
	// Inform handler that we are waiting for capabilities from that node and that user.
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/psi");
	CPPUNIT_ASSERT (m_handler->hasVersion(context));

	Tag *query = getCorrectQuery();
	
	// CapabilityHandler should connect user when he's ready for it.
	m_user->setConnected(false);
	m_user->setReadyForConnect(true);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, context);
	CPPUNIT_ASSERT (m_user->isConnected());
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_ROSTERX, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_FILETRANSFER, "psi") == true);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_CHATSTATES, "psi") == true);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_XHTML_IM, "psi") == true);
}

void CapabilityHandlerTest::handleDiscoInfoNoWait() {
	CPPUNIT_ASSERT (m_handler->hasVersion(2) == false);

	Tag *query = getCorrectQuery();
	
	// CapabilityHandler should connect user when he's ready for it.
	m_user->setConnected(false);
	m_user->setReadyForConnect(true);
	m_user->setResource("psi", 50, 0);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, 2);
	CPPUNIT_ASSERT (m_user->isConnected() == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_ROSTERX, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_FILETRANSFER, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_CHATSTATES, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_XHTML_IM, "psi") == false);
}

void CapabilityHandlerTest::handleDiscoInfoCapsSet() {
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/psi");
	CPPUNIT_ASSERT (m_handler->hasVersion(context));

	Tag *query = getCorrectQuery();
	
	// CapabilityHandler should connect user when he's ready for it.
	m_user->setConnected(false);
	m_user->setReadyForConnect(true);
	m_user->setResource("psi", 50, GLOOX_FEATURE_ROSTERX);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, context);
	CPPUNIT_ASSERT (m_user->isConnected() == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_ROSTERX, "psi") == true);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_FILETRANSFER, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_CHATSTATES, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_XHTML_IM, "psi") == false);
}

void CapabilityHandlerTest::handleDiscoInfoNotReady() {
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/psi");
	CPPUNIT_ASSERT (m_handler->hasVersion(context));

	Tag *query = getCorrectQuery();
	
	// CapabilityHandler should connect user when he's ready for it.
	m_user->setConnected(false);
	m_user->setReadyForConnect(false);
	m_user->setResource("psi", 50, GLOOX_FEATURE_ROSTERX);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, context);
	CPPUNIT_ASSERT (m_user->isConnected() == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_ROSTERX, "psi") == true);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_FILETRANSFER, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_CHATSTATES, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_XHTML_IM, "psi") == false);
}

void CapabilityHandlerTest::handleDiscoInfoNoUser() {
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/psi");
	CPPUNIT_ASSERT (m_handler->hasVersion(context));

	Tag *query = getCorrectQuery();
	
	// just tests segfault :-P
	Transport::instance()->userManager()->removeUser(m_user);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, context);
	m_user = new TestingUser("user@example.com", "user@example.com");
	Transport::instance()->userManager()->addUser(m_user);
}

void CapabilityHandlerTest::handleDiscoInfoBadIdentity() {
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/psi");
	CPPUNIT_ASSERT (m_handler->hasVersion(context));

	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "http://jabber.org/protocol/disco#info");
	query->addAttribute("node", "http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=");
	
	Tag *identity = new Tag("identity");
	identity->addAttribute("category", "not-client");
	identity->addAttribute("name", "Exodus 0.9.1");
	identity->addAttribute("type", "pc");
	query->addChild(identity);

	m_user->setConnected(false);
	m_user->setReadyForConnect(true);
	m_user->setResource("psi", 50, 0);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, context);

	CPPUNIT_ASSERT (m_user->isConnected() == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_ROSTERX, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_FILETRANSFER, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_CHATSTATES, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_XHTML_IM, "psi") == false);
}


void CapabilityHandlerTest::handleDiscoInfoNoIdentity() {
	int context = m_handler->waitForCapabilities("http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=", "user@example.com/psi");
	CPPUNIT_ASSERT (m_handler->hasVersion(context));
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "http://jabber.org/protocol/disco#info");
	query->addAttribute("node", "http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0=");

	m_user->setConnected(false);
	m_user->setReadyForConnect(true);
	m_handler->handleDiscoInfo(JID("user@example.com/psi"), query, context);

	CPPUNIT_ASSERT (m_user->isConnected() == true);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_ROSTERX, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_FILETRANSFER, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_CHATSTATES, "psi") == false);
	CPPUNIT_ASSERT (m_user->hasFeature(GLOOX_FEATURE_XHTML_IM, "psi") == false);
}
