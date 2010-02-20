#include "spectrumconversationtest.h"
#include "spectrumconversation.h"
#include "transport.h"
#include "../capabilityhandler.h"

void SpectrumConversationTest::up (void) {
	m_user = new TestingUser("key", "user@example.com");
	m_conv = new SpectrumConversation(NULL, SPECTRUM_CONV_CHAT);
}

void SpectrumConversationTest::down (void) {
	delete m_conv;
	delete m_user;
}

void SpectrumConversationTest::handleMessage() {
	std::string r;
	r = "<message to='user@example.com' from='user1%example.com@icq.localhost/bot' type='chat'>"
			"<body>Hi, how are you? I'm fine.</body>"
		"</message>";

	m_conv->handleMessage(m_user, "user1@example.com/psi", "Hi, how are you? <b>I'm fine</b>.", PURPLE_MESSAGE_RECV, time(NULL));

	testTagCount(1);
	compare(r);
}

void SpectrumConversationTest::handleMessageDelay() {
	std::string r;
	r = "<message to='user@example.com' from='user1%example.com@icq.localhost/bot' type='chat'>"
			"<body>Hi, how are you? I&apos;m fine.</body>"
			"<html xmlns='http://jabber.org/protocol/xhtml-im'>"
				"<body xmlns='http://www.w3.org/1999/xhtml'>"
				"Hi, how are you? <span style='font-weight: bold;'>I&apos;m fine</span>."
				"</body>"
			"</html>"
			"<delay xmlns='urn:xmpp:delay' from='user1%example.com@icq.localhost/bot' stamp='1970-01-02T10:17:36Z'>"
			"</delay>"
		"</message>";

	m_user->setResource("gajim", 55, GLOOX_FEATURE_XHTML_IM);
	m_conv->handleMessage(m_user, "user1@example.com/psi", "<body>Hi, how are you? <b>I'm fine</b>.</body>", PURPLE_MESSAGE_RECV, 123456);

	testTagCount(1);
	compare(r);
}

