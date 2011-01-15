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

#include "identica.h"
#include "../main.h"
#include "../transport.h"

IdenticaProtocol::IdenticaProtocol() {
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("jabber:iq:gateway");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
// 	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

IdenticaProtocol::~IdenticaProtocol() {}

bool IdenticaProtocol::isValidUsername(const std::string &str){
	return true;
}

std::list<std::string> IdenticaProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> IdenticaProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string IdenticaProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your Identica username and password:");
	else if (key == "username")
		return _("Identica username");
	return "not defined";
}

void IdenticaProtocol::onUserCreated(AbstractUser *user) {
	PurpleValue *value;
	if ( (value = user->getSetting("twitter_oauth_secret")) == NULL ) {
		Transport::instance()->sql()->addSetting(user->storageId(), "twitter_oauth_secret", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("twitter_oauth_secret"), value);
	}
	if ( (value = user->getSetting("twitter_oauth_token")) == NULL ) {
		Transport::instance()->sql()->addSetting(user->storageId(), "twitter_oauth_token", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("twitter_oauth_token"), value);
	}
	if ( (value = user->getSetting("twitter_sent_msg_ids")) == NULL ) {
		Transport::instance()->sql()->addSetting(user->storageId(), "twitter_sent_msg_ids", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("twitter_sent_msg_ids"), value);
	}
	if ( (value = user->getSetting("twitter_last_msg_id")) == NULL ) {
		Transport::instance()->sql()->addSetting(user->storageId(), "twitter_last_msg_id", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("twitter_last_msg_id"), value);
	}
}

void IdenticaProtocol::onDestroy(AbstractUser *user) {
	PurpleAccount *account = user->account();
	if (account == NULL)
		return;
	PurpleValue *value = NULL;
	if ( (value = user->getSetting("twitter_oauth_secret")) != NULL ) {
		purple_value_set_string(value, purple_account_get_string(account, "twitter_oauth_secret", ""));
		user->updateSetting("twitter_oauth_secret", value);
	}
	if ( (value = user->getSetting("twitter_oauth_token")) != NULL ) {
		purple_value_set_string(value, purple_account_get_string(account, "twitter_oauth_token", ""));
		user->updateSetting("twitter_oauth_token", value);
	}
	if ( (value = user->getSetting("twitter_sent_msg_ids")) != NULL ) {
		purple_value_set_string(value, purple_account_get_string(account, "twitter_sent_msg_ids", ""));
		user->updateSetting("twitter_sent_msg_ids", value);
	}
	if ( (value = user->getSetting("twitter_last_msg_id")) != NULL ) {
		purple_value_set_string(value, purple_account_get_string(account, "twitter_last_msg_id", ""));
		user->updateSetting("twitter_last_msg_id", value);
	}
}

void IdenticaProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	AbstractUser *user = (AbstractUser *) account->ui_data;
	PurpleValue *value = NULL;
	if ( (value = user->getSetting("twitter_oauth_secret")) != NULL ) {
		purple_account_set_string(account, "twitter_oauth_secret", purple_value_get_string(value));
	}
	if ( (value = user->getSetting("twitter_oauth_token")) != NULL ) {
		purple_account_set_string(account, "twitter_oauth_token", purple_value_get_string(value));
	}
	if ( (value = user->getSetting("twitter_sent_msg_ids")) != NULL ) {
		purple_account_set_string(account, "twitter_sent_msg_ids", purple_value_get_string(value));
	}
	if ( (value = user->getSetting("twitter_last_msg_id")) != NULL ) {
		purple_account_set_string(account, "twitter_last_msg_id", purple_value_get_string(value));
	}
}

SPECTRUM_PROTOCOL(identica, IdenticaProtocol)
