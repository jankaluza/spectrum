#include "spectrummucconversationtest.h"
#include "spectrummucconversation.h"
#include "transport.h"
#include "../caps.h"

void SpectrumMUCConversationTest::setUp (void) {
	m_user = new TestingUser("key", "user@example.com");
	m_conv = new SpectrumMUCConversation(NULL, "#room%irc.freenode.net@icq.localhost", "psi");
}

void SpectrumMUCConversationTest::tearDown (void) {
 	delete m_conv;
 	delete m_user;
	
 	while (m_tags.size() != 0) {
 		Tag *tag = m_tags.front();
 		m_tags.remove(tag);
 		delete tag;
 	}
	Transport::instance()->clearTags();
}

void SpectrumMUCConversationTest::handleMessage() {
	m_conv->handleMessage(m_user, "Frank", "Hi, how are you?", PURPLE_MESSAGE_RECV, time(NULL));
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 message stanza sent", m_tags.size() == 1);

	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "message");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com/psi");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "#room%irc.freenode.net@icq.localhost/Frank");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "groupchat");
	CPPUNIT_ASSERT (tag->findChild("body") != NULL);
	CPPUNIT_ASSERT (tag->findChild("body")->cdata() == "Hi, how are you?");

	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
}

void SpectrumMUCConversationTest::addUsers() {
	GList *cbuddies = NULL;
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_none", NULL, PURPLE_CBFLAGS_NONE));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_voice", NULL, PURPLE_CBFLAGS_VOICE));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_halfop", NULL, PURPLE_CBFLAGS_HALFOP));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_op", NULL, PURPLE_CBFLAGS_OP));
	cbuddies = g_list_prepend(cbuddies, purple_conv_chat_cb_new("Frank_founder", NULL, PURPLE_CBFLAGS_FOUNDER));
	m_conv->addUsers(m_user, cbuddies);
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 6 message stanza sent", m_tags.size() == 5);
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com/psi");
		CPPUNIT_ASSERT (tag->findChild("x") != NULL);
		CPPUNIT_ASSERT (tag->findChild("x")->findChild("item") != NULL);
		if (tag->findAttribute("from") == "#room%irc.freenode.net@icq.localhost/Frank_founder") {
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("affiliation") == "owner");
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("role") == "moderator");
		}
		else if (tag->findAttribute("from") == "#room%irc.freenode.net@icq.localhost/Frank_voice") {
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("affiliation") == "member");
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("role") == "participant");
		}
		else if (tag->findAttribute("from") == "#room%irc.freenode.net@icq.localhost/Frank_halfop") {
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("affiliation") == "admin");
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("role") == "moderator");
		}
		else if (tag->findAttribute("from") == "#room%irc.freenode.net@icq.localhost/Frank_op") {
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("affiliation") == "admin");
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("role") == "moderator");
		}
		else if (tag->findAttribute("from") == "#room%irc.freenode.net@icq.localhost/Frank_none") {
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("affiliation") == "member");
			CPPUNIT_ASSERT (tag->findChild("x")->findChild("item")->findAttribute("role") == "participant");
		}
		else
			CPPUNIT_ASSERT_MESSAGE ("Presence for unknown user", 1 == 0);
		m_tags.remove(tag);
		delete tag;
	}
	Transport::instance()->clearTags();
}

void SpectrumMUCConversationTest::renameUser() {
	m_conv->renameUser(m_user, "Frank", "Bob", "Bob the King");
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 2 presence stanzas sent", m_tags.size() == 2);
	
	// TODO: write test here
	
	delete m_tags.front();
	delete m_tags.back();
	m_tags.clear();
	Transport::instance()->clearTags();
}
