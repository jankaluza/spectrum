#include "spectrumconversationtest.h"
#include "spectrumconversation.h"
#include "transport.h"
#include "../caps.h"

void SpectrumConversationTest::setUp (void) {
	m_user = new TestingUser("key", "user@example.com");
	m_conv = new SpectrumConversation(NULL, SPECTRUM_CONV_CHAT);
}

void SpectrumConversationTest::tearDown (void) {
 	delete m_conv;
 	delete m_user;
	
 	while (m_tags.size() != 0) {
 		Tag *tag = m_tags.front();
 		m_tags.remove(tag);
 		delete tag;
 	}
	Transport::instance()->clearTags();
}

void SpectrumConversationTest::handleMessage() {
	m_conv->handleMessage(m_user, "user1@example.com/psi", "Hi, how are you? <b>I'm fine</b>.", PURPLE_MESSAGE_RECV, time(NULL));
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 message stanza sent", m_tags.size() == 1);

	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "message");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "chat");
	CPPUNIT_ASSERT (tag->findChild("body") != NULL);
	CPPUNIT_ASSERT (tag->findChild("body")->cdata() == "Hi, how are you? I'm fine.");
	CPPUNIT_ASSERT (tag->findChild("html") == NULL);

	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
	
	
	m_user->setResource("gajim", 55, GLOOX_FEATURE_XHTML_IM);
	m_conv->handleMessage(m_user, "user1@example.com/psi", "<body>Hi, how are you? <b>I'm fine</b>.</body>", PURPLE_MESSAGE_RECV, 123456);
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 message stanza sent", m_tags.size() == 1);
	//<message to='user@example.com' from='user1%example.com@icq.localhost/bot' type='chat'>
	// <body>Hi, how are you? I&apos;m fine.</body>
	// <delay xmlns='urn:xmpp:delay' from='user1%example.com@icq.localhost/bot' stamp='1970-01-02T10:17:36Z'/>
	// <html xmlns='http://jabber.org/protocol/xhtml-im'>
	// <body xmlns='http://www.w3.org/1999/xhtml'>Hi, how are you? <b>I&apos;m fine</b>.</body></html></message>
	tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "message");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "chat");
	CPPUNIT_ASSERT (tag->findChild("body") != NULL);
	CPPUNIT_ASSERT (tag->findChild("body")->cdata() == "Hi, how are you? I'm fine.");
	CPPUNIT_ASSERT (tag->findChild("html", "xmlns", "http://jabber.org/protocol/xhtml-im") != NULL);
	CPPUNIT_ASSERT (tag->findChild("html")->findChild("body") != NULL);
	CPPUNIT_ASSERT (tag->findChild("html")->findChild("body")->findAttribute("xmlns") == "http://www.w3.org/1999/xhtml");
	CPPUNIT_ASSERT (tag->findChild("html")->findChild("body")->xml() == "<body xmlns='http://www.w3.org/1999/xhtml'>Hi, how are you? <b>I&apos;m fine</b>.</body>");
	CPPUNIT_ASSERT (tag->findChild("delay") != NULL);
	CPPUNIT_ASSERT (tag->findChild("delay")->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findChild("delay")->findAttribute("stamp") == "1970-01-02T10:17:36Z");
	
	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
}

