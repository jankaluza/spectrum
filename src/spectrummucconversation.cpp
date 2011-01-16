/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "spectrummucconversation.h"
#include "main.h" // just for replaceBadJidCharacters
#include "transport.h"
#include "usermanager.h"

SpectrumMUCConversation::SpectrumMUCConversation(PurpleConversation *conv, const std::string &jid, const std::string &resource) : AbstractConversation(SPECTRUM_CONV_GROUPCHAT) {
	m_jid = jid;
	m_res = "/" + resource;
	m_conv = conv;
	m_connected = false;
	m_lastPresence = NULL;
#ifndef TESTS
	m_conv->ui_data = this;
	PurpleConvChat *chat = purple_conversation_get_chat_data(m_conv);
	m_nickname = purple_conv_chat_get_nick(chat);
#endif
}

SpectrumMUCConversation::~SpectrumMUCConversation() {
}

void SpectrumMUCConversation::handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime, const std::string &currentBody) {
	std::string name(who);
// 
// 	// send message to user
// 	std::string message(purple_unescape_html(msg));
// 	Message s(Message::Groupchat, user->jid() + m_res, message);
// 	s.setFrom(m_jid + "/" + name);
// 
// 	Transport::instance()->send( s.tag() );

	// Escape HTML characters.
	char *newline = purple_strdup_withhtml(msg);
	char *strip, *xhtml;
	purple_markup_html_to_xhtml(newline, &xhtml, &strip);
	std::string message(strip);

	std::string to = user->jid() + m_res;
	std::cout << user->jid() << " " << m_res << "\n";
	
	Message s(Message::Groupchat, to, message);
	s.setFrom(m_jid + "/" + name);

// 	<delay xmlns="urn:xmpp:delay" stamp="2010-02-16T15:49:19Z"/>
// 	<x xmlns="jabber:x:delay" stamp="20100216T15:49:19"/>
	
	// Delayed messages, we have to count with some delay
	if ((unsigned long) time(NULL)-10 > (unsigned long) mtime) {
		char buf[80];
		strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&mtime));
		std::string timestamp(buf);
		DelayedDelivery *d = new DelayedDelivery(m_jid, timestamp);
		s.addExtension(d);
	}

	Tag *stanzaTag = s.tag();
	g_free(newline);
	g_free(xhtml);
	g_free(strip);

// 	std::string res = getResource();
// 	if (user->hasFeature(GLOOX_FEATURE_XHTML_IM, res) && m != message) {
// 		Transport::instance()->parser()->getTag("<body>" + m + "</body>", sendXhtmlTag, stanzaTag);
// 		return;
// 	}
	Transport::instance()->send(stanzaTag);

}

void SpectrumMUCConversation::addUsers(AbstractUser *user, GList *cbuddies) {
	Log(m_jid, "ADDING USERS3");
	GList *l = cbuddies;
	while (l != NULL) {
		PurpleConvChatBuddy *cb = (PurpleConvChatBuddy *)l->data;
// 		std::string alias(cb->alias ? cb->alias: "");
		std::string name(cb->name);
		int flags = GPOINTER_TO_INT(cb->flags);
// 		PURPLE_CBFLAGS_OP
// 		<presence
// 		from='darkcave@chat.shakespeare.lit/firstwitch'
// 		to='hag66@shakespeare.lit/pda'>
// 		<x xmlns='http://jabber.org/protocol/muc#user'>
// 		<item affiliation='member' role='participant'/>
// 		</x>
// 		</presence>
		Tag *tag = new Tag("presence");
		tag->addAttribute("from", m_jid + "/" + name);
		tag->addAttribute("to", user->jid() + m_res);

		Tag *x = new Tag("x");
		x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

		Tag *item = new Tag("item");


		if (flags & PURPLE_CBFLAGS_OP || flags & PURPLE_CBFLAGS_HALFOP) {
			item->addAttribute("affiliation", "admin");
			item->addAttribute("role", "moderator");
		}
		else if (flags & PURPLE_CBFLAGS_FOUNDER) {
			item->addAttribute("affiliation", "owner");
			item->addAttribute("role", "moderator");
		}
		else {
			item->addAttribute("affiliation", "member");
			item->addAttribute("role", "participant");
		}

		std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
		if (name == m_nickname) {
			Tag *status = new Tag("status");
			status->addAttribute("code", "110");
			item->addChild(status);
		}

		x->addChild(item);
		tag->addChild(x);

		if (name == m_nickname) {
			if (m_lastPresence)
				delete m_lastPresence;
			m_lastPresence = tag->clone();
		}
		Transport::instance()->send(tag);

		l = l->next;
	}
	if (!m_connected && !m_topic.empty()) {
		sendTopic(user);
	}
	m_connected = true;
}

void SpectrumMUCConversation::renameUser(AbstractUser *user, const char *old_name, const char *new_name, const char *new_alias) {
	std::string oldName(old_name);
	std::string newName(new_name);
	Tag *tag = new Tag("presence");
	tag->addAttribute("from", m_jid + "/" + oldName);
	tag->addAttribute("to", user->jid() + m_res);
	tag->addAttribute("type", "unavailable");

	Tag *x = new Tag("x");
	x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

	Tag *item = new Tag("item");
	item->addAttribute("affiliation", "member");
	item->addAttribute("role", "participant");
	item->addAttribute("nick", newName);

	Tag *status = new Tag("status");
	status->addAttribute("code","303");

	x->addChild(item);
	x->addChild(status);
	tag->addChild(x);
	Transport::instance()->send(tag);


	tag = new Tag("presence");
	tag->addAttribute("from", m_jid + "/" + newName);
	tag->addAttribute("to", user->jid() + m_res);

	x = new Tag("x");
	x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

	item = new Tag("item");
	item->addAttribute("affiliation", "member");
	item->addAttribute("role", "participant");

	x->addChild(item);
	tag->addChild(x);
	Transport::instance()->send(tag);
}

void SpectrumMUCConversation::removeUsers(AbstractUser *_user, GList *users) {
	GList *l;
	for (l = users; l != NULL; l = l->next) {
		std::string user((char *)l->data);
		Tag *tag = new Tag("presence");
		tag->addAttribute("from", m_jid + "/" + user);
		tag->addAttribute("to", _user->jid() + m_res);
		tag->addAttribute("type", "unavailable");

		Tag *x = new Tag("x");
		x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

		Tag *item = new Tag("item");
		item->addAttribute("affiliation", "member");
		item->addAttribute("role", "participant");

		x->addChild(item);
		tag->addChild(x);
		Transport::instance()->send(tag);
	}
}

void SpectrumMUCConversation::changeTopic(AbstractUser *user, const char *who, const char *topic) {
	m_topic = topic ? topic : "";
	m_topicUser = who ? who : m_jid.substr(0,m_jid.find('%'));
// 	if (user->connected())
	sendTopic(user);
}

void SpectrumMUCConversation::sendTopic(AbstractUser *user) {
	Tag *m = new Tag("message");
	m->addAttribute("from", m_jid + "/" + m_topicUser);
	m->addAttribute("to", user->jid() + m_res);
	m->addAttribute("type", "groupchat");
	m->addChild( new Tag("subject", m_topic) );

	Transport::instance()->send(m);
}
