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

void IdenticaProtocol::onUserCreated(User *user) {
	user->addSetting<std::string>("twitter_oauth_secret", "");
	user->addSetting<std::string>("twitter_oauth_token", "");
	user->addSetting<std::string>("twitter_sent_msg_ids", "");
	user->addSetting<std::string>("twitter_last_msg_id", "");
}

void IdenticaProtocol::onDestroy(User *user) {
	PurpleAccount *account = user->account();
	if (account == NULL)
		return;
	user->updateSetting<std::string>("twitter_oauth_secret", purple_account_get_string(account, "twitter_oauth_secret", ""));
	user->updateSetting<std::string>("twitter_oauth_token", purple_account_get_string(account, "twitter_oauth_token", ""));
	user->updateSetting<std::string>("twitter_sent_msg_ids", purple_account_get_string(account, "twitter_sent_msg_ids", ""));
	user->updateSetting<std::string>("twitter_last_msg_id", purple_account_get_string(account, "twitter_last_msg_id", ""));
}

void IdenticaProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	User *user = (User *) account->ui_data;
	purple_account_set_string(account, "twitter_oauth_secret", user->getSetting<std::string>("twitter_oauth_secret", "").c_str());
	purple_account_set_string(account, "twitter_oauth_token", user->getSetting<std::string>("twitter_oauth_token", "").c_str());
	purple_account_set_string(account, "twitter_sent_msg_ids", user->getSetting<std::string>("twitter_sent_msg_ids", "").c_str());
	purple_account_set_string(account, "twitter_last_msg_id", user->getSetting<std::string>("twitter_last_msg_id", "").c_str());
}

SPECTRUM_PROTOCOL(identica, IdenticaProtocol)
