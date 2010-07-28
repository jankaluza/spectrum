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
		if (s_conv->getType() == SPECTRUM_CONV_GROUPCHAT) {
			// TODO: send unavailable presences for MUCs here
		}
		delete s_conv;
	}
	m_conversations.clear(); 
}

bool SpectrumMessageHandler::isOpenedConversation(const std::string &name) {
	return m_conversations.find(name) != m_conversations.end();
}

bool SpectrumMessageHandler::isMUC(const std::string &key) {
	return m_mucs_names.find(key) != m_mucs_names.end();
}

void SpectrumMessageHandler::handlePurpleMessage(PurpleAccount* account, char * name, char *msg, PurpleConversation *conv, PurpleMessageFlags flags) {
	if (conv == NULL) {
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, name);
#ifndef TESTS
		if (Transport::instance()->getConfiguration().protocol == "irc") {
			m_conversations[getConversationName(conv)] = new SpectrumConversation(conv, SPECTRUM_CONV_GROUPCHAT);
			m_conversations[getConversationName(conv)]->setKey(getConversationName(conv));
		}
		else if (Transport::instance()->getConfiguration().protocol == "xmpp") {
			std::string jid = purple_conversation_get_name(conv);
			Transport::instance()->protocol()->makeRoomJID(m_user, jid);
			if (m_mucs_names.find(jid) != m_mucs_names.end()) {
				std::string key = jid + "/" + JID(std::string(name)).resource();
				m_conversations[key] = new SpectrumConversation(conv, SPECTRUM_CONV_GROUPCHAT, jid);
				m_conversations[key]->setKey(key);
			}
			else {
				addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_CHAT));
			}
		}
		else
			addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_CHAT));
#endif
	}
}

void SpectrumMessageHandler::addConversation(PurpleConversation *conv, AbstractConversation *s_conv, const std::string &key) {
	std::string k = key.empty() ? getConversationName(conv) : key;
	Log(m_user->jid()," Adding Conversation; key: " << k);
	s_conv->setKey(k);
	m_conversations[k] = s_conv;
	if (s_conv->getType() == SPECTRUM_CONV_GROUPCHAT) {
		m_mucs_names[k] = 1;
		m_mucs++;
	}
}

void SpectrumMessageHandler::removeConversation(const std::string &name, bool only_muc) {
	if (!isOpenedConversation(name))
		return;
	if (only_muc && m_conversations[name]->getType() != SPECTRUM_CONV_GROUPCHAT)
		return;
	PurpleConversation *conv = m_conversations[name]->getConv();
	purple_conversation_destroy(conv);

	// conversation should be removed/destroyed in purpleConversationDestroyed, but just to be sure...
	if (!isOpenedConversation(name))
		return;
	Log("WARNING", "Conversation is not removed after purple_conversation_destroy (removing now): " << name);
	if (m_conversations[name]->getType() == SPECTRUM_CONV_GROUPCHAT)
		m_mucs--;
	delete m_conversations[name];
	m_conversations.erase(name);

}

void SpectrumMessageHandler::handleWriteIM(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL)
		return;

	// Don't forwards our own messages.
	if (flags & PURPLE_MESSAGE_SEND || flags & PURPLE_MESSAGE_SYSTEM)
		return;

	AbstractConversation *s_conv = (AbstractConversation *) conv->ui_data;
	if (s_conv) {
		std::string name(who);
		// If it's IM for GROUPCHAT conversation, it has to be PM, so we have to let protocol to generate proper username for us.
		// This covers situations where who="spectrum@conferece.spectrum.im/HanzZ", but we need name="HanzZ"
		if (s_conv->getType() == SPECTRUM_CONV_GROUPCHAT) {
			Transport::instance()->protocol()->makeUsernameRoom(m_user, name);
		}
		s_conv->handleMessage(m_user, name.c_str(), msg, flags, mtime, m_currentBody);
	}
	else {
		Log("WARNING", "handleWriteIM: No AbstractConversation set for given 'conv':" << who);
	}
}

void SpectrumMessageHandler::handleWriteChat(PurpleConversation *conv, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime) {
	if (who == NULL || flags & PURPLE_MESSAGE_SYSTEM)
		return;

	AbstractConversation *s_conv = getSpectrumMUCConversation(conv);
	s_conv->handleMessage(m_user, who, msg, flags, mtime, m_currentBody);
}

void SpectrumMessageHandler::purpleChatAddUsers(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) {
	AbstractConversation *s_conv = getSpectrumMUCConversation(conv);
	s_conv->addUsers(m_user, cbuddies);
}

void SpectrumMessageHandler::purpleChatTopicChanged(PurpleConversation *conv, const char *who, const char *topic) {
	AbstractConversation *s_conv = getSpectrumMUCConversation(conv);
	s_conv->changeTopic(m_user, who, topic);
}

void SpectrumMessageHandler::purpleChatRenameUser(PurpleConversation *conv, const char *old_name, const char *new_name, const char *new_alias) {
	AbstractConversation *s_conv = getSpectrumMUCConversation(conv);
	s_conv->renameUser(m_user, old_name, new_name, new_alias);
}

void SpectrumMessageHandler::purpleChatRemoveUsers(PurpleConversation *conv, GList *users) {
	AbstractConversation *s_conv = getSpectrumMUCConversation(conv);
	s_conv->removeUsers(m_user, users);
}

void SpectrumMessageHandler::purpleConversationDestroyed(PurpleConversation *conv) {
	AbstractConversation *s_conv = (AbstractConversation *) conv->ui_data;
	if (!s_conv) {
		Log("WARNING", "purpleConversationDestroyed called for conversation without ui_data");
		return;
	}
	std::string name = s_conv->getKey();
	if (!isOpenedConversation(name)) {
		Log("WARNING", "purpleConversationDestroyed called for unregistered conversation with name " << name);
		return;
	}
	Log("purpleConversationDestroyed", "Removing conversation data: " << name);
	if (m_conversations[name]->getType() == SPECTRUM_CONV_GROUPCHAT)
		m_mucs--;
	delete m_conversations[name];
	m_conversations.erase(name);
}

void SpectrumMessageHandler::handleMessage(const Message& msg) {
	PurpleConversation * conv;

	std::string key = msg.to().full();	// key for m_conversations
	std::string username = msg.to().full();
	std::string room = "";
	// Translates JID to Purple username
	// This code covers for example situations where username = "#pidgin%irc.freenode.org@irc.spectrum.im",
	// but we need username = "#pidgin".
	if (isMUC(msg.to().bare())) {
		room = msg.to().bare();
		Transport::instance()->protocol()->makePurpleUsernameRoom(m_user, msg.to(), username);
	}
	else {
		Transport::instance()->protocol()->makePurpleUsernameIM(m_user, msg.to(), username);
	}
	
	Log("SpectrumMessageHandler::handleMessage", "username " << username << " key " << key);
	// open new conversation or get the opened one
	if (!isOpenedConversation(key)) {
		// if normalized username is empty, it's broken username...
		std::string normalized(purple_normalize(m_user->account(), username.c_str()));
		if (normalized.empty()) {
			Log("SpectrumMessageHandler::handleMessage", "WARNING username is empty after normalization:" << username);
			return;
		}
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, m_user->account() , username.c_str());
#ifndef TESTS
		addConversation(conv, new SpectrumConversation(conv, SPECTRUM_CONV_CHAT, room), key);
#endif
		m_conversations[key]->setResource(msg.from().resource());
	}
	else {
		conv = m_conversations[key]->getConv();
		m_conversations[key]->setResource(msg.from().resource());
	}
	m_currentBody = msg.body();

	// Handles commands (TODO: move me to commands.cpp)
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

		std::string sender = purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM ? purple_conversation_get_name(conv) : "transport";
		switch (status) {
			case PURPLE_CMD_STATUS_OK:
				break;
			case PURPLE_CMD_STATUS_NOT_FOUND:
				{
					purple_conversation_write(conv, sender.c_str(), tr(m_user->getLang(),_("Transport: Unknown command.")), PURPLE_MESSAGE_RECV, time(NULL));
					break;
				}
			case PURPLE_CMD_STATUS_WRONG_ARGS:
				purple_conversation_write(conv, sender.c_str(), tr(m_user->getLang(),_("Syntax Error: Wrong number of arguments.")), PURPLE_MESSAGE_RECV, time(NULL));
				break;
			case PURPLE_CMD_STATUS_FAILED:
				purple_conversation_write(conv, sender.c_str(), tr(m_user->getLang(),error ? error : _("The command failed for an unknown reason.")), PURPLE_MESSAGE_RECV, time(NULL));
				break;
			case PURPLE_CMD_STATUS_WRONG_TYPE:
				if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)
					purple_conversation_write(conv, sender.c_str(), tr(m_user->getLang(),_("That command only works in group chats, not 1:1 conversations.")), PURPLE_MESSAGE_RECV, time(NULL));
				else
					purple_conversation_write(conv, sender.c_str(), tr(m_user->getLang(),_("That command only works in 1:1 conversations, not group chats.")), PURPLE_MESSAGE_RECV, time(NULL));
				break;
			case PURPLE_CMD_STATUS_WRONG_PRPL:
				if (m_currentBody.find("/me") == 0) {
					handled = false;
				}
				else {
					purple_conversation_write(conv, sender.c_str(), tr(m_user->getLang(),_("That command is not supported for this legacy network.")), PURPLE_MESSAGE_RECV, time(NULL));
				}
				break;
		}

		if (error)
			g_free(error);
		if (handled)
			return;
	}

	// escape and send
	gchar *_markup = purple_markup_escape_text(m_currentBody.c_str(), -1);
	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		purple_conv_im_send(PURPLE_CONV_IM(conv), _markup);
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
	Log("hasOpenedMUC", m_mucs);
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
		std::cout << "getconversationName: " << name << "\n";
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
		Transport::instance()->protocol()->makeRoomJID(m_user, name);
	return name;
}

AbstractConversation *SpectrumMessageHandler::getSpectrumMUCConversation(PurpleConversation *conv) {
	AbstractConversation *s_conv = (AbstractConversation *) conv->ui_data;
	if (s_conv) {
		return s_conv;
	}
	else {
#ifndef TESTS
		std::string jid = purple_conversation_get_name(conv);
		Transport::instance()->protocol()->makeRoomJID(m_user, jid);
		std::cout << "CONVNAME:" << purple_conversation_get_name(conv) << "\n";
		s_conv = (AbstractConversation *) new SpectrumMUCConversation(conv, jid, m_user->getRoomResource(purple_conversation_get_name(conv)));
		addConversation(conv, s_conv);
#endif
	}
	return s_conv;
}
