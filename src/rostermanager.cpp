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

#include "rostermanager.h"
#include "abstractspectrumbuddy.h"
#include "main.h"
#include "log.h"
#include "usermanager.h"
#include "caps.h"
#include "spectrumtimer.h"
#include "transport.h"

struct SendPresenceToAllData {
	RosterManager *manager;
	std::string to;
};

static void sendUnavailablePresence(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SendPresenceToAllData *d = (SendPresenceToAllData *) data;
// 	RosterManager *manager = d->manager;
	std::string &to = d->to;

	if (s_buddy->isOnline()) {
		Tag *tag = new Tag("presence");
		tag->addAttribute( "to", to );
		tag->addAttribute( "type", "unavailable" );
		tag->addAttribute( "from", s_buddy->getJid());
		Transport::instance()->send( tag );
		Transport::instance()->userManager()->buddyOffline();
		s_buddy->setOffline();
	}
}

static void sendCurrentPresence(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SendPresenceToAllData *d = (SendPresenceToAllData *) data;
	RosterManager *manager = d->manager;
	std::string &to = d->to;
	if (s_buddy->isOnline()) {
		PurpleBuddy *buddy = s_buddy->getBuddy();
		if (buddy) {
			Tag *tag = manager->generatePresenceStanza(buddy);
			if (tag) {
				tag->addAttribute("to", to);
				Transport::instance()->send( tag );
			}
		}
	}
}

RosterManager::RosterManager(AbstractUser *user) {
	m_user = user;
}

RosterManager::~RosterManager() {
	g_hash_table_destroy(m_roster);
}

/*
 * Returns true if user with UIN `name` and subscription `subscription` is
 * in roster. If subscription.empty(), returns true on any subsciption.
 */
bool RosterManager::isInRoster(const std::string &name, const std::string &subscription) {
// 	std::map<std::string,RosterRow>::iterator iter = m_roster.begin();
// 	iter = m_roster.find(name);
// 	if (iter != m_roster.end()) {
// 		if (subscription.empty())
// 			return true;
// 		if (m_roster[name].subscription == subscription)
// 			return true;
// 	}

	if (g_hash_table_lookup(m_roster, name.c_str()))
		return true;
	return false;
}

/*
 * Generates presence stanza for PurpleBuddy `buddy`. This will
 * create whole <presence> stanza without 'to' attribute.
 */
Tag *RosterManager::generatePresenceStanza(PurpleBuddy *buddy) {
	if (buddy == NULL)
		return NULL;
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	return s_buddy->generatePresenceStanza(m_user->getFeatures());
}

void RosterManager::sendUnavailablePresenceToAll() {
	SendPresenceToAllData *data = new SendPresenceToAllData;
	data->manager = this;
	data->to = m_user->jid();
	g_hash_table_foreach(m_roster, sendUnavailablePresence, data);
	delete data;
}

void RosterManager::sendPresenceToAll(const std::string &to) {
	SendPresenceToAllData *data = new SendPresenceToAllData;
	data->manager = this;
	data->to = to;
	g_hash_table_foreach(m_roster, sendCurrentPresence, data);
	delete data;
}

void RosterManager::removeFromLocalRoster(const std::string &uin) {
	if (!g_hash_table_lookup(m_roster, uin.c_str()))
		return;
	g_hash_table_remove(m_roster, uin.c_str());
}

AbstractSpectrumBuddy *RosterManager::getRosterItem(const std::string &uin) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) g_hash_table_lookup(m_roster, uin.c_str());
	return s_buddy;
}

void RosterManager::addRosterItem(PurpleBuddy *buddy) {
	if (g_hash_table_lookup(m_roster, purple_buddy_get_name(buddy)))
		return;
	if (buddy->node.ui_data)
		g_hash_table_replace(m_roster, g_strdup(purple_buddy_get_name(buddy)), buddy->node.ui_data);
	else
		Log(std::string(purple_buddy_get_name(buddy)), "This buddy has not set AbstractSpectrumBuddy!!!");
}

void RosterManager::setRoster(GHashTable *roster) {
	m_roster = roster;
}

