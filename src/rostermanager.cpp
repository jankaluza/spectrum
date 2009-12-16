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
	int features;
	std::string to;
};

static void sendUnavailablePresence(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SendPresenceToAllData *d = (SendPresenceToAllData *) data;
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
	int features = d->features;
	std::string &to = d->to;
	if (s_buddy->isOnline()) {
		Tag *tag = s_buddy->generatePresenceStanza(features);
		if (tag) {
			tag->addAttribute("to", to);
			Transport::instance()->send( tag );
		}
	}
}

static gboolean sync_cb(gpointer data) {
	RosterManager *manager = (RosterManager *) data;
	return manager->syncBuddiesCallback();
}

RosterManager::RosterManager(AbstractUser *user) {
	m_user = user;
	m_syncTimer = new SpectrumTimer(12000, &sync_cb, this);
	m_subscribeLastCount = -1;
	m_roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

RosterManager::~RosterManager() {
	g_hash_table_destroy(m_roster);
}

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

void RosterManager::sendUnavailablePresenceToAll() {
	SendPresenceToAllData *data = new SendPresenceToAllData;
	data->features = m_user->getFeatures();
	data->to = m_user->jid();
	g_hash_table_foreach(m_roster, sendUnavailablePresence, data);
	delete data;
}

void RosterManager::sendPresenceToAll(const std::string &to) {
	SendPresenceToAllData *data = new SendPresenceToAllData;
	data->features = m_user->getFeatures();
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

void RosterManager::addRosterItem(AbstractSpectrumBuddy *s_buddy) {
	if (isInRoster(s_buddy->getName()))
		return;
	g_hash_table_replace(m_roster, g_strdup(s_buddy->getName().c_str()), s_buddy);
}

void RosterManager::addRosterItem(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy;
	if (s_buddy)
		addRosterItem(s_buddy);
	else
		Log(std::string(purple_buddy_get_name(buddy)), "This buddy has not set AbstractSpectrumBuddy!!!");
}

void RosterManager::setRoster(GHashTable *roster) {
	if (m_roster)
		g_hash_table_destroy(m_roster);
	m_roster = roster;
}

void RosterManager::sendPresence(AbstractSpectrumBuddy *s_buddy, const std::string &resource) {
	Tag *tag = s_buddy->generatePresenceStanza(m_user->getFeatures());
	if (tag) {
		tag->addAttribute("to", m_user->jid() + std::string(resource.empty() ? "" : "/" + resource));
		Transport::instance()->send(tag);
	}
}

void RosterManager::sendPresence(const std::string &name, const std::string &resource) {
	AbstractSpectrumBuddy *s_buddy = getRosterItem(name);
	if (s_buddy) {
		sendPresence(s_buddy, resource);
	}
	else {
		std::string n(name);
		std::for_each( n.begin(), n.end(), replaceBadJidCharacters() );
		Log(m_user->jid(), "answering to probe presence with unavailable presence");
		Tag *tag = new Tag("presence");
		tag->addAttribute("to", m_user->jid() + std::string(resource.empty() ? "" : "/" + resource));
		tag->addAttribute("from", n + "@" + Transport::instance()->jid());
		tag->addAttribute("type", "unavailable");
		Transport::instance()->send(tag);
	}
}

void RosterManager::handleBuddySignedOn(AbstractSpectrumBuddy *s_buddy) {
	sendPresence(s_buddy);
	s_buddy->setOnline();
}

void RosterManager::handleBuddySignedOn(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddySignedOn(s_buddy);
}

void RosterManager::handleBuddySignedOff(AbstractSpectrumBuddy *s_buddy) {
	sendPresence(s_buddy);
	s_buddy->setOffline();
}

void RosterManager::handleBuddySignedOff(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddySignedOff(s_buddy);
}

void RosterManager::handleBuddyStatusChanged(AbstractSpectrumBuddy *s_buddy, PurpleStatus *status, PurpleStatus *old_status) {
	sendPresence(s_buddy);
	s_buddy->setOnline();
}

void RosterManager::handleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddyStatusChanged(s_buddy, status, old_status);
}

void RosterManager::handleBuddyRemoved(AbstractSpectrumBuddy *s_buddy) {
	m_subscribeCache.erase(s_buddy->getName());
}

void RosterManager::handleBuddyRemoved(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddyRemoved(s_buddy);
}

void RosterManager::handleBuddyCreated(AbstractSpectrumBuddy *s_buddy) {
	std::string alias = s_buddy->getAlias();
	std::string name = s_buddy->getName();
	if (name.empty())
		return;

	Log(m_user->jid(), "handleBuddyCreated: " << name << " ("<< alias <<")");

	if (!isInRoster(name, "")) {
		m_syncTimer->start();
		Log(m_user->jid(), "Not in roster => adding to subscribe cache");
		m_subscribeCache[name] = s_buddy;
// 		if (m_user->findResourceWithFeature(GLOOX_FEATURE_ROSTERX)) {
// 			Log(m_jid, "Not in roster => sending rosterx");
// 			m_subscribeCache[name] = s_buddy;
// 		}
// 		else {
// 			Log(m_jid, "Not in roster => sending subscribe");
// 			Tag *tag = new Tag("presence");
// 			tag->addAttribute("type", "subscribe");
// 			tag->addAttribute("from", s_buddy->getJid());
// 			tag->addAttribute("to", m_jid);
// 			Tag *nick = new Tag("nick", alias);
// 			nick->addAttribute("xmlns","http://jabber.org/protocol/nick");
// 			tag->addChild(nick);
// 			p->j->send(tag);
// 		}
	}
}

bool RosterManager::syncBuddiesCallback() {
	Log(m_user->jid(), "sync_cb lastCount: " << m_subscribeLastCount << "cacheSize: " << int(m_subscribeCache.size()));
	if (m_subscribeLastCount == int(m_subscribeCache.size())) {
		sendNewBuddies();
		return false;
	}
	else {
		m_subscribeLastCount = int(m_subscribeCache.size());
		return true;
	}
}

void RosterManager::sendNewBuddies() {
	if (int(m_subscribeCache.size()) == 0)
		return;

	Log(m_user->jid(), "Sending rosterX");

	Resource res = m_user->findResourceWithFeature(GLOOX_FEATURE_ROSTERX);
	if (res) {
		Tag *tag = new Tag("iq");
		tag->addAttribute("to", m_user->jid() + "/" + res.name);
		tag->addAttribute("type", "set");
		tag->addAttribute("id", Transport::instance()->getId());
		tag->addAttribute("from", Transport::instance()->jid());
		Tag *x = new Tag("x");
		x->addAttribute("xmlns", "http://jabber.org/protocol/rosterx");
		
		Tag *item;
		std::map<std::string, AbstractSpectrumBuddy *>::iterator it = m_subscribeCache.begin();
		while (it != m_subscribeCache.end()) {
			AbstractSpectrumBuddy *s_buddy = (*it).second;
			std::string jid = s_buddy->getBareJid();
			std::string alias = s_buddy->getAlias();

			addRosterItem(s_buddy);

			item = new Tag("item");
			item->addAttribute("action", "add");
			item->addAttribute("jid", jid);
			item->addAttribute("name", alias);
			item->addChild( new Tag("group", s_buddy->getGroup()));
			x->addChild(item);
			it++;
		}
		tag->addChild(x);
		Log("rosterx stanza", tag->xml() << "\n");
		Transport::instance()->send(tag);
	}
	else {
		std::map<std::string, AbstractSpectrumBuddy *>::iterator it = m_subscribeCache.begin();
		while (it != m_subscribeCache.end()) {
			AbstractSpectrumBuddy *s_buddy = (*it).second;
			std::string alias = s_buddy->getAlias();

			Tag *tag = new Tag("presence");
			tag->addAttribute("type", "subscribe");
			tag->addAttribute("from", s_buddy->getJid());
			tag->addAttribute("to", Transport::instance()->jid());
			Tag *nick = new Tag("nick", alias);
			nick->addAttribute("xmlns","http://jabber.org/protocol/nick");
			tag->addChild(nick);
			Transport::instance()->send(tag);

			addRosterItem(s_buddy);
			it++;
		}
	}

	m_subscribeCache.clear();
	m_subscribeLastCount = -1;
}

void RosterManager::handlePresence(const Presence &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag)
		return;

	// Probe presence
	if (stanza.subtype() == Presence::Probe && stanza.to().username() != "") {
		std::string name(stanza.to().username());
		std::for_each( name.begin(), name.end(), replaceJidCharacters() );
		sendPresence(name);
	}
}
