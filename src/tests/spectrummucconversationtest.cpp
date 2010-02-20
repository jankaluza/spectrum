#include "spectrummucconversationtest.h"
#include "spectrummucconversation.h"
#include "transport.h"
#include "../capabilityhandler.h"

void SpectrumMUCConversationTest::setUp (void) {
	m_user = new TestingUser("key", "user@example.com");
	m_conv = new SpectrumMUCConversation(NULL, "#room%irc.freenode.net@icq.localhost", "psi");
}

void SpectrumMUCConversationTest::tearDown (void) {
	delete m_conv;
	delete m_user;
}

void SpectrumMUCConversationTest::handleMessage() {
	std::string r;
	r =	"<message to='user@example.com/psi' from='#room%irc.freenode.net@icq.localhost/Frank' type='groupchat'>"
			"<body>Hi, how are you?</body>"
		"</message>";
	m_conv->handleMessage(m_user, "Frank", "Hi, how are you?", PURPLE_MESSAGE_RECV, time(NULL));
	testTagCount(1);
	compare(r);
}

void SpectrumMUCConversationTest::addUsers() {
	std::string r_founder;
	std::string r_op;
	std::string r_halfop;
	std::string r_voice;
	std::string r_none;
	r_founder =	"<presence from='#room%irc.freenode.net@icq.localhost/Frank_founder' to='user@example.com/psi'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<item affiliation='owner' role='moderator'/>"
					"</x>"
				"</presence>";
	r_op =		"<presence from='#room%irc.freenode.net@icq.localhost/Frank_op' to='user@example.com/psi'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<item affiliation='admin' role='moderator'/>"
					"</x>"
				"</presence>";
	r_halfop =	"<presence from='#room%irc.freenode.net@icq.localhost/Frank_halfop' to='user@example.com/psi'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<item affiliation='admin' role='moderator'/>"
					"</x>"
				"</presence>";
	r_voice =	"<presence from='#room%irc.freenode.net@icq.localhost/Frank_voice' to='user@example.com/psi'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<item affiliation='member' role='participant'/>"
					"</x>"
				"</presence>";
	r_none =	"<presence from='#room%irc.freenode.net@icq.localhost/Frank_voice' to='user@example.com/psi'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<item affiliation='member' role='participant'/>"
					"</x>"
				"</presence>";
	GList *cbuddies = NULL;
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_none", NULL, PURPLE_CBFLAGS_NONE));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_voice", NULL, PURPLE_CBFLAGS_VOICE));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_halfop", NULL, PURPLE_CBFLAGS_HALFOP));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_op", NULL, PURPLE_CBFLAGS_OP));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_founder", NULL, PURPLE_CBFLAGS_FOUNDER));
	m_conv->addUsers(m_user, cbuddies);
	
	testTagCount(6);
	compare(r_founder);
	compare(r_op);
	compare(r_halfop);
	compare(r_voice);
	compare(r_none);
}
/*
void SpectrumMUCConversationTest::renameUser() {
	m_conv->renameUser(m_user, "Frank", "Bob", "Bob the King");
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 2 presence stanzas sent", m_tags.size() == 2);
	
	// TODO: write test here
	
	delete m_tags.front();
	delete m_tags.back();
	m_tags.clear();
	Transport::instance()->clearTags();
}*/
