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

#include "spectrummessagehandler.h"
#include "abstractspectrumbuddy.h"
#include "main.h"
#include "log.h"
#include "usermanager.h"
#include "capabilityhandler.h"
#include "spectrumtimer.h"
#include "transport.h"
#include "gloox/chatstate.h"
#include "parser.h"

#ifndef TESTS
#include "spectrumconversation.h"
#include "spectrummucconversation.h"
#include "user.h"
#endif

SpectrumMessageHandler::SpectrumMessageHandler(AbstractUser *user) {
	m_user = user;
	m_mucs = 0;
	m_currentBody = "";
}

SpectrumMessageHandler::~SpectrumMessageHandler() {
	for (std::map<std::string, AbstractConversation *>::iterator i = m_conversations.begin(); i != m_conversations.end(); i++) {
		AbstractConversation *s_conv = (*i).second;
		delete s_conv;
	}
	m_conversations.clear(); 
}

bool SpectrumMessageHandler::isOpenedConversation(const std::string &name) {
	std::map<std::string, AbstractConversation *>::iterator iter = m_conversations.begin();
	iter = m_conversations.find(name);
	return iter != m_conversations.end();
}

void SpectrumMessageHandler::handlePurpleMessage(PurpleAccount* account, char * name, char *msg, PurpleConversation *conv, PurpleMessageFlags flags) {
	if (conv == NULL) {
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, name);
#ifndef TESTS
		if (Transport::instance()->getConfiguration().protocol == "irc")
// 			addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_GROUPCHAT));
			m_conversations[getConversationName(conv)] = new SpectrumConversation(conv, SPECTRUM_CONV_GROUPCHAT);
		else
			addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_CHAT));
#endif
	}
}

void SpectrumMessageHandler::addConversation(PurpleConversation *conv, AbstractConversation *s_conv, const std::string &key) {
	std::string k = key.empty() ? getConversationName(conv) : key;
	Log(m_user->jid()," Adding Conversation; name: " << k);
	m_conversations[k] = s_conv;
	if (s_conv->getType() == SPECTRUM_CONV_GROUPCHAT) {
		m_mucs_names[k] = 1;
		m_mucs++;
	}
}

void SpectrumMessageHandler::removeConversation(const std::string &name) {
	if (!isOpenedConversation(name))
		return;
	if (m_conversations[name]->getType() == SPECTRUM_CONV_GROUPCHAT)
		m_mucs--;
	PurpleConversation *conv = m_conversations[name]->getConv();
	purple_conversation_destroy(conv);
	delete m_conversations[name];
	m_conversations.erase(name);
}

void SpectrumMessageHandler::handleWriteIM(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL)
		return;

	// Don't resend our own messages.
	if (flags & PURPLE_MESSAGE_SEND || flags & PURPLE_MESSAGE_SYSTEM)
		return;
	
	std::string name = getConversationName(conv);
	std::cout << "NAME1 " << name << "\n";
	if (!isOpenedConversation(name)) {
#ifndef TESTS
		addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_CHAT));
#endif
	}

// 	if (Transport::instance()->getConfiguration().protocol == "irc") {
// 		char *who_final = g_strdup_printf("%s%%%s", who, JID(m_user->username()).server().c_str());
// 		m_conversations[name]->handleMessage(m_user, who_final, msg, flags, mtime, m_currentBody);
// 		g_free(who_final);
// 	}
// 	else
	m_conversations[name]->handleMessage(m_user, who, msg, flags, mtime, m_currentBody);
}

void SpectrumMessageHandler::handleWriteChat(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL || flags & PURPLE_MESSAGE_SYSTEM)
		return;

	std::string name = getSpectrumMUCConversation(conv);
	m_conversations[name]->handleMessage(m_user, who, msg, flags, mtime, m_currentBody);
}

void SpectrumMessageHandler::purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	std::string name = getSpectrumMUCConversation(conv);
	m_conversations[name]->addUsers(m_user, cbuddies);
}

void SpectrumMessageHandler::purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic) {
	std::string name = getSpectrumMUCConversation(conv);
	m_conversations[name]->changeTopic(m_user, who, topic);
}

void SpectrumMessageHandler::purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	std::string name = getSpectrumMUCConversation(conv);
	m_conversations[name]->renameUser(m_user, old_name, new_name, new_alias);
}

void SpectrumMessageHandler::purpleChatRemoveUsers(PurpleConversation *conv, GList *users) {
	std::string name = getSpectrumMUCConversation(conv);
	m_conversations[name]->removeUsers(m_user, users);
}

/*
 * Received new message from jabber user.
 */
void SpectrumMessageHandler::handleMessage(const Message& msg) {
	PurpleConversation * conv;
	
	std::string room = "";
	std::string k = purpleUsername(msg.to().username());
// 	std::for_each( k.begin(), k.end(), replaceJidCharacters() );
	if (m_mucs_names.find(k) != m_mucs_names.end())
		room = msg.to().username();
	
	std::string username = Transport::instance()->protocol()->prepareName(m_user, msg.to());
	std::cout << "MESSAGE USERNAME " << username << "\n";
// 	std::transform(convName.begin(), convName.end(), convName.begin(),(int(*)(int)) std::tolower);
	
// 	if (Transport::instance()->getConfiguration().protocol == "irc") {
// 		if (!isOpenedConversation(username)) {
// 			username += "%" + JID(m_user->username()).server();
// 		}
// 	}
	
	Log("SpectrumMessageHandler::handleMessage", "username " << username);
	// open new conversation or get the opened one
	if (!isOpenedConversation(username)) {
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, m_user->account() , username.c_str());
#ifndef TESTS
		addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_CHAT, room), username);
#endif
		m_conversations[username]->setResource(msg.from().resource());
	}
	else {
		conv = m_conversations[username]->getConv();
		m_conversations[username]->setResource(msg.from().resource());
	}
	m_currentBody = msg.body();

	if (m_currentBody.find("/") == 0) {
		bool handled = true;
		PurpleCmdStatus status;
		char *error = NULL;
		std::string command = m_currentBody;
		if (command.find("/transport") == 0)
			command.erase(0, 11);
		else
			command.erase(0, 1);
		status = purple_cmd_do_command(conv, command.c_str(), command.c_str(), &error);

		switch (status) {
			case PURPLE_CMD_STATUS_OK:
				break;
			case PURPLE_CMD_STATUS_NOT_FOUND:
				{
					purple_conversation_write(conv, "transport", tr(m_user->getLang(),_("Transport: Unknown command.")), PURPLE_MESSAGE_RECV, time(NULL));
					break;
				}
			case PURPLE_CMD_STATUS_WRONG_ARGS:
				purple_conversation_write(conv, "transport", tr(m_user->getLang(),_("Syntax Error: Wrong number of arguments.")), PURPLE_MESSAGE_RECV, time(NULL));
				break;
			case PURPLE_CMD_STATUS_FAILED:
				purple_conversation_write(conv, "transport", tr(m_user->getLang(),error ? error : _("The command failed for an unknown reason.")), PURPLE_MESSAGE_RECV, time(NULL));
				break;
			case PURPLE_CMD_STATUS_WRONG_TYPE:
				if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)
					purple_conversation_write(conv, "transport", tr(m_user->getLang(),_("That command only works in group chats, not 1:1 conversations.")), PURPLE_MESSAGE_RECV, time(NULL));
				else
					purple_conversation_write(conv, "transport", tr(m_user->getLang(),_("That command only works in 1:1 conversations, not group chats.")), PURPLE_MESSAGE_RECV, time(NULL));
				break;
			case PURPLE_CMD_STATUS_WRONG_PRPL:
				if (m_currentBody.find("/me") == 0) {
					handled = false;
				}
				else {
					purple_conversation_write(conv, "transport", tr(m_user->getLang(),_("That command is not supported for this legacy network.")), PURPLE_MESSAGE_RECV, time(NULL));
				}
				break;
		}

		if (error)
			g_free(error);
		if (handled)
			return;
	}

	// send this message
	gchar *_markup = g_markup_escape_text(m_currentBody.c_str(), -1);

	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		PurpleConvIm *im = purple_conversation_get_im_data(conv);
		purple_conv_im_send(im, _markup);
	}
	else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
		purple_conv_chat_send(PURPLE_CONV_CHAT(conv), _markup);
	}
	g_free(_markup);
}
void SpectrumMessageHandler::removeConversationResource(const std::string &resource) {
	for (std::map<std::string, AbstractConversation *>::iterator u = m_conversations.begin(); u != m_conversations.end() ; u++) {
		if ((*u).second->getResource() == resource) {
			(*u).second->setResource("");
		}
	}
}

bool SpectrumMessageHandler::hasOpenedMUC() {
	return m_mucs != 0;
}

void SpectrumMessageHandler::handleChatState(const std::string &uin, const std::string &state) {
	if (!m_user->hasFeature(GLOOX_FEATURE_CHATSTATES) || !m_user->hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	Log(m_user->jid(), "Sending " << state << " message to " << uin);
	if (state == "composing")
		serv_send_typing(purple_account_get_connection(m_user->account()),uin.c_str(),PURPLE_TYPING);
	else if (state == "paused")
		serv_send_typing(purple_account_get_connection(m_user->account()),uin.c_str(),PURPLE_TYPED);
	else
		serv_send_typing(purple_account_get_connection(m_user->account()),uin.c_str(),PURPLE_NOT_TYPING);
}


std::string SpectrumMessageHandler::getConversationName(PurpleConversation *conv) {
	std::string name(purple_conversation_get_name(conv));
	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		// Remove resource if it's XMPP JID
		size_t pos = name.find("/");
		if (pos != std::string::npos)
			name.erase((int) pos, name.length() - (int) pos);

// 		std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
		AbstractSpectrumBuddy *s_buddy = NULL;
#ifndef TESTS
		User *u = (User *) m_user;
		s_buddy = u->getRosterItem(name);
#endif
		if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
			name = JID::escapeNode(name);
		else
			std::for_each( name.begin(), name.end(), replaceBadJidCharacters() ); // OK
		if (Transport::instance()->getConfiguration().protocol != "irc") {
			std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
		}
	}
	else
		name = Transport::instance()->protocol()->prepareRoomName(m_user, name);
	return name;
}

std::string SpectrumMessageHandler::getSpectrumMUCConversation(PurpleConversation *conv) {
	std::string name = getConversationName(conv);

	if (!isOpenedConversation(name)) {
#ifndef TESTS
		std::string resource = purple_conversation_get_name(conv);
		std::transform(resource.begin(), resource.end(), resource.begin(),(int(*)(int)) std::tolower);
		std::string name_safe = name;
		std::for_each( name_safe.begin(), name_safe.end(), replaceBadJidCharacters() ); // OK
		std::string j = name_safe + "@" + Transport::instance()->jid();
		SpectrumMUCConversation *conversation = new SpectrumMUCConversation(conv, j, m_user->getRoomResource(resource));
		addConversation(conv, conversation);
#endif
	}
	return name;
}
