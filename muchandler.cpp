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

#include "muchandler.h"
#include "main.h"
#include "user.h"

MUCHandler::MUCHandler(User *user, const std::string &jid, const std::string &userJid){
	m_user = user;
	m_jid = jid;
	m_userJid = userJid;
	m_connected = false;
	m_topic = "";
	m_topicUser = "";
}

MUCHandler::~MUCHandler() {}

Tag * MUCHandler::handlePresence(const Presence &stanza) {
	Tag *ret;
	Tag *stanzaTag = stanza.tag();
	ret = handlePresence(stanzaTag);
	delete stanzaTag;
	return ret;
}

Tag * MUCHandler::handlePresence(Tag *stanza) {
	if (stanza->findAttribute("type") != "unavailable") {
		GHashTable *comps = NULL;
		std::string name = JID(stanza->findAttribute("to")).username();
		PurpleConnection *gc = purple_account_get_connection(m_user->account());
		if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
			if (name.find("%") != std::string::npos)
				name = name.substr(0, name.find("%"));
			comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
		}
		if (comps) {
			serv_join_chat(gc, comps);
		}
	}
	else if (m_connected) {
		purple_conversation_destroy(m_conv);
	}
	return NULL;
}

void MUCHandler::addUsers(GList *cbuddies) {
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
		tag->addAttribute("to", m_userJid);

		Tag *x = new Tag("x");
		x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

		Tag *item = new Tag("item");


		if (flags & PURPLE_CBFLAGS_OP) {
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
		m_user->p->j->send(tag);

		l = l->next;
	}
	if (!m_connected && !m_topic.empty()) {
		sendTopic();
	}
	m_connected = true;
}

void MUCHandler::messageReceived(const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	std::string name(who);

	// send message to user
	std::string message(purple_unescape_html(msg));
	Message s(Message::Groupchat, m_userJid, message);
	s.setFrom(m_jid + "/" + name);

	m_user->p->j->send( s );
}

void MUCHandler::renameUser(const char *old_name, const char *new_name, const char *new_alias) {
// <presence
// from='darkcave@chat.shakespeare.lit/oldhag'
// to='wiccarocks@shakespeare.lit/laptop'>
// <x xmlns='http://jabber.org/protocol/muc#user'>
// <item affiliation='member' role='participant'/>
// </x>
// </presence>
	std::string oldName(old_name);
	std::string newName(new_name);
	Tag *tag = new Tag("presence");
	tag->addAttribute("from", m_jid + "/" + oldName);
	tag->addAttribute("to", m_userJid);
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
	m_user->p->j->send(tag);


	tag = new Tag("presence");
	tag->addAttribute("from", m_jid + "/" + newName);
	tag->addAttribute("to", m_userJid);

	x = new Tag("x");
	x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

	item = new Tag("item");
	item->addAttribute("affiliation", "member");
	item->addAttribute("role", "participant");

	x->addChild(item);
	tag->addChild(x);
	m_user->p->j->send(tag);

}

void MUCHandler::removeUsers(GList *users) {
	GList *l;
	for (l = users; l != NULL; l = l->next) {
		std::string user((char *)l->data);
		Tag *tag = new Tag("presence");
		tag->addAttribute("from", m_jid + "/" + user);
		tag->addAttribute("to", m_userJid);
		tag->addAttribute("type", "unavailable");

		Tag *x = new Tag("x");
		x->addAttribute("xmlns", "http://jabber.org/protocol/muc#user");

		Tag *item = new Tag("item");
		item->addAttribute("affiliation", "member");
		item->addAttribute("role", "participant");

		x->addChild(item);
		tag->addChild(x);
		m_user->p->j->send(tag);
	}
}

void MUCHandler::sendTopic() {
	Tag *m = new Tag("message");
	m->addAttribute("from", m_jid + "/" + m_topicUser);
	m->addAttribute("to", m_userJid);
	m->addAttribute("type", "groupchat");
	m->addChild( new Tag("subject", m_topic) );

	m_user->p->j->send(m);
}

void MUCHandler::topicChanged(const char *who, const char *topic) {
// <message
// from='wiccarocks@shakespeare.lit/laptop'
// to='darkcave@chat.shakespeare.lit'
// type='groupchat'>
// <subject>Fire Burn and Cauldron Bubble!</subject>
// </message>
	m_topic = std::string(topic);
	m_topicUser = std::string(who ? who : "transport");
	if (m_connected)
		sendTopic();
}

