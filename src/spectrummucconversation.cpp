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
	m_resource = "/" + resource;
	m_conv = conv;
}

SpectrumMUCConversation::~SpectrumMUCConversation() {
}

void SpectrumMUCConversation::handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime, const std::string &currentBody) {
	std::string name(who);

	// send message to user
	std::string message(purple_unescape_html(msg));
	Message s(Message::Groupchat, user->jid() + m_resource, message);
	s.setFrom(m_jid + "/" + name);

	Transport::instance()->send( s.tag() );
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
		tag->addAttribute("to", user->jid() + m_resource);

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
	tag->addAttribute("to", user->jid() + m_resource);
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
	tag->addAttribute("to", user->jid() + m_resource);

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
		tag->addAttribute("to", _user->jid() + m_resource);
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
	m_topic = std::string(topic);
	m_topicUser = std::string(who ? who : "transport");
// 	if (user->connected())
	sendTopic(user);
}

void SpectrumMUCConversation::sendTopic(AbstractUser *user) {
	Tag *m = new Tag("message");
	m->addAttribute("from", m_jid + "/" + m_topicUser);
	m->addAttribute("to", user->jid() + m_resource);
	m->addAttribute("type", "groupchat");
	m->addChild( new Tag("subject", m_topic) );

	Transport::instance()->send(m);
}
