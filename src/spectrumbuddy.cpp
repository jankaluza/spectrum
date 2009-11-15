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

#include "spectrumbuddy.h"
#include "main.h"
#include "user.h"
#include "log.h"
#include "sql.h"
#include "usermanager.h"
#include "caps.h"
#include "striphtmltags.h"

void SpectrumBuddyFree(gpointer data) {
	SpectrumBuddy *buddy = (SpectrumBuddy *) data;
	delete buddy;
}

static void save_settings(gpointer k, gpointer v, gpointer data) {
	PurpleValue *value = (PurpleValue *) v;
	std::string key((char *) k);
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) data;
	User *user = s_buddy->getUser();
	long id = s_buddy->getId();
	if (purple_value_get_type(value) == PURPLE_TYPE_BOOLEAN) {
		if (purple_value_get_boolean(value))
			user->p->sql()->addBuddySetting(user->storageId(), id, key, "1", purple_value_get_type(value));
		else
			user->p->sql()->addBuddySetting(user->storageId(), id, key, "0", purple_value_get_type(value));
	}
	else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
		user->p->sql()->addBuddySetting(user->storageId(), id, key, purple_value_get_string(value), purple_value_get_type(value));
	}
}

SpectrumBuddy::SpectrumBuddy() : m_id(0), m_uin(""), m_nickname(""), m_group("Buddies"),
	m_subscription(SUBSCRIPTION_NONE), m_online(false), m_lastPresence(""), m_buddy(NULL) {
}

SpectrumBuddy::SpectrumBuddy(const std::string &uin) : m_id(0), m_uin(uin), m_nickname(""), m_group("Buddies"),
	m_subscription(SUBSCRIPTION_NONE), m_online(false), m_lastPresence(""), m_buddy(NULL) {
}

SpectrumBuddy::SpectrumBuddy(User *user, PurpleBuddy *buddy) : m_id(0), m_uin(""), m_nickname(""), m_group("Buddies"),
	m_subscription(SUBSCRIPTION_NONE), m_online(false), m_lastPresence(""), m_buddy(NULL) {

	setUser(m_user);
	setBuddy(buddy);
}

SpectrumBuddy::~SpectrumBuddy() {
	// Remove pointer to this class from PurpleBuddy ui_data
	if (m_buddy) {
		SpectrumBuddy *buddy = (SpectrumBuddy *) m_buddy->node.ui_data;
		if (buddy)
			m_buddy->node.ui_data = NULL;
	}
}

void SpectrumBuddy::setUser(User *user) {
	m_user = user;
}

User *SpectrumBuddy::getUser() {
	return m_user;
}

void SpectrumBuddy::setBuddy(PurpleBuddy *buddy) {
	if (m_user)
		Log("setBuddy", m_user->jid());
	if (buddy) {
		if (buddy->node.ui_data == NULL) {
			buddy->node.ui_data = (void *) this;
		}
	}
	m_buddy = buddy;
	Log("setting buddy", getUin());
}

void SpectrumBuddy::setOffline() {
	m_online = false;
}

void SpectrumBuddy::setOnline() {
	m_online = true;
}

bool SpectrumBuddy::isOnline() {
	return m_online;
}

void SpectrumBuddy::setId(long id) {
	m_id = id;
}

long SpectrumBuddy::getId() {
	return m_id;
}

void SpectrumBuddy::setUin(const std::string &uin) {
	m_uin = uin;
}

const std::string &SpectrumBuddy::getUin() {
	return m_uin;
}

const std::string SpectrumBuddy::getSafeUin() {
	std::string u(getUin());
	std::for_each( u.begin(), u.end(), replaceBadJidCharacters() );
	return u;
}

void SpectrumBuddy::setSubscription(SpectrumSubscriptionType subscription) {
	m_subscription = subscription;
}

SpectrumSubscriptionType &SpectrumBuddy::getSubscription() {
	return m_subscription;
}

void SpectrumBuddy::setNickname(const std::string &nickname) {
	m_nickname = nickname;
}

const std::string &SpectrumBuddy::getNickname() {
	if (m_buddy) {
		if (purple_buddy_get_server_alias(m_buddy))
			m_nickname = (std::string) purple_buddy_get_server_alias(m_buddy);
		else
			m_nickname = (std::string) purple_buddy_get_alias(m_buddy);
	}
	return m_nickname;
}

void SpectrumBuddy::setGroup(const std::string &group) {
	m_group = group;
}

const std::string &SpectrumBuddy::getGroup() {
	return m_group;
}

const std::string SpectrumBuddy::getJid() {
	return getBareJid() + "/bot";
}

const std::string SpectrumBuddy::getBareJid() {
	return getSafeUin() + "@" + GlooxMessageHandler::instance()->jid();
}

bool SpectrumBuddy::getStatus(int &status, std::string &message) {
	if (m_buddy == NULL)
		return false;
	PurplePresence *pres = purple_buddy_get_presence(m_buddy);
	if (pres == NULL)
		return false;
	PurpleStatus *stat = purple_presence_get_active_status(pres);
	if (stat == NULL)
		return false;
	status = purple_status_type_get_primitive(purple_status_get_type(stat));

	const char *statusMessage = purple_status_get_attr_string(stat, "message");
	message = statusMessage ? std::string(statusMessage) : "";
	message = stripHTMLTags(message);
	return true;
}

void SpectrumBuddy::store(PurpleBuddy *buddy) {
	if (buddy && !m_buddy)
		setBuddy(buddy);
	if (!m_buddy)
		return;
	if (!m_user)
		return;
	std::string alias(getNickname());
	std::string name(getUin());
	if (m_id != 0) {
		GlooxMessageHandler::instance()->sql()->addBuddy(m_user->storageId(), name, "both", purple_group_get_name(purple_buddy_get_group(m_buddy)) ? std::string(purple_group_get_name(purple_buddy_get_group(m_buddy))) : std::string("Buddies"), alias);
	}
	else {
		m_id = GlooxMessageHandler::instance()->sql()->addBuddy(m_user->storageId(), name, "both", purple_group_get_name(purple_buddy_get_group(m_buddy)) ? std::string(purple_group_get_name(purple_buddy_get_group(m_buddy))) : std::string("Buddies"), alias);
		m_buddy->node.ui_data = (void *) this;
	}
	Log("SpectrumBuddy::store", m_id << " " << name << " " << alias);
	g_hash_table_foreach(m_buddy->node.settings, save_settings, this);
}

void SpectrumBuddy::remove() {
	Log("SpectrumBuddy::remove", m_id);
	if (!m_buddy)
		return;
	if (!m_user)
		return;
	if (m_id == 0)
		return;
	Log("SpectrumBuddy::remove1", m_id);
	m_buddy->node.ui_data = NULL;
	GlooxMessageHandler::instance()->sql()->removeBuddy(m_user->storageId(), getUin(), m_id);
	purple_account_remove_buddy(m_user->account(), m_buddy, purple_buddy_get_group(m_buddy));
	purple_blist_remove_buddy(m_buddy);
	m_buddy = NULL;
}
