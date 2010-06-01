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
#include "capabilityhandler.h"
#include "spectrumtimer.h"
#include "transport.h"

#ifndef TESTS
#include "spectrumbuddy.h"
#endif

struct SendPresenceToAllData {
	int features;
	std::string to;
	bool markOffline;
};

static void handleAskSubscriptionBuddies(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SpectrumRosterManager *manager = (SpectrumRosterManager *) data;

	if (s_buddy->getSubscription() == "ask") {
		manager->handleBuddyCreated(s_buddy);
	}
}

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
		if (d->markOffline)
			s_buddy->setOffline();
	}
}

static void sendCurrentPresence(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SendPresenceToAllData *d = (SendPresenceToAllData *) data;
	int features = d->features;
	std::string &to = d->to;
	std::cout << "online: " << s_buddy->isOnline() << "\n";
	if (s_buddy->isOnline()) {
		Tag *tag = s_buddy->generatePresenceStanza(features);
		std::cout << "online: " << tag << "\n";
		if (tag) {
			tag->addAttribute("to", to);
			Transport::instance()->send( tag );
		}
	}
}

static gboolean sync_cb(gpointer data) {
	SpectrumRosterManager *manager = (SpectrumRosterManager *) data;
	return manager->syncBuddiesCallback();
}

SpectrumRosterManager::SpectrumRosterManager(AbstractUser *user) : RosterStorage(user) {
	m_user = user;
	m_syncTimer = new SpectrumTimer(12000, &sync_cb, this);
	m_subscribeLastCount = -1;
	m_roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	m_loadingFromDB = false;
}

SpectrumRosterManager::~SpectrumRosterManager() {
	g_hash_table_destroy(m_roster);
	delete m_syncTimer;
}

bool SpectrumRosterManager::isInRoster(const std::string &name, const std::string &subscription) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) g_hash_table_lookup(m_roster, name.c_str());

	if (s_buddy) {
		if (subscription.empty())
			return true;
		return subscription == s_buddy->getSubscription();
	}
	return false;
}

void SpectrumRosterManager::sendUnavailablePresenceToAll(const std::string &resource) {
	SendPresenceToAllData *data = new SendPresenceToAllData;
	data->features = m_user->getFeatures();
	if (resource.empty()) {
		data->to = m_user->jid();
		data->markOffline = true;
	}
	else {
		data->to = m_user->jid() + "/" + resource;
		data->markOffline = false;
	}
	g_hash_table_foreach(m_roster, sendUnavailablePresence, data);
	delete data;
}

void SpectrumRosterManager::sendPresenceToAll(const std::string &to) {
	SendPresenceToAllData *data = new SendPresenceToAllData;
	data->features = m_user->getFeatures();
	data->to = to;
	g_hash_table_foreach(m_roster, sendCurrentPresence, data);
	delete data;
}

void SpectrumRosterManager::removeFromLocalRoster(const std::string &uin) {
	if (!g_hash_table_lookup(m_roster, uin.c_str()))
		return;
	g_hash_table_remove(m_roster, uin.c_str());
}

AbstractSpectrumBuddy *SpectrumRosterManager::getRosterItem(const std::string &uin) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) g_hash_table_lookup(m_roster, uin.c_str());
	return s_buddy;
}

void SpectrumRosterManager::addRosterItem(AbstractSpectrumBuddy *s_buddy) {
	if (isInRoster(s_buddy->getName()))
		return;
	g_hash_table_replace(m_roster, g_strdup(s_buddy->getName().c_str()), s_buddy);
}

void SpectrumRosterManager::addRosterItem(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	if (s_buddy)
		addRosterItem(s_buddy);
}

void SpectrumRosterManager::setRoster(GHashTable *roster) {
	if (m_roster)
		g_hash_table_destroy(m_roster);
	m_roster = roster;
	g_hash_table_foreach(m_roster, handleAskSubscriptionBuddies, this);
}

void SpectrumRosterManager::loadRoster() {
	m_loadingFromDB = true;
	setRoster(Transport::instance()->sql()->getBuddies(m_user->storageId(), m_user->account()));
	m_loadingFromDB = false;
}

void SpectrumRosterManager::sendPresence(AbstractSpectrumBuddy *s_buddy, const std::string &resource, bool only_new) {
	Tag *tag = s_buddy->generatePresenceStanza(m_user->getFeatures(), only_new);
	if (tag) {
		tag->addAttribute("to", m_user->jid() + std::string(resource.empty() ? "" : "/" + resource));
		Transport::instance()->send(tag);
	}
}

void SpectrumRosterManager::sendPresence(const std::string &name, const std::string &resource) {
	AbstractSpectrumBuddy *s_buddy = getRosterItem(name);
	if (s_buddy) {
		sendPresence(s_buddy, resource);
	}
	else {
		std::string n(name);
		std::for_each( n.begin(), n.end(), replaceBadJidCharacters() ); // THIS ONE IS RIGHT
		Log(m_user->jid(), "answering to probe presence with unavailable presence");
		Tag *tag = new Tag("presence");
		tag->addAttribute("to", m_user->jid() + std::string(resource.empty() ? "" : "/" + resource));
		tag->addAttribute("from", n + "@" + Transport::instance()->jid());
		tag->addAttribute("type", "unavailable");
		Transport::instance()->send(tag);
	}
}

void SpectrumRosterManager::handleBuddySignedOn(AbstractSpectrumBuddy *s_buddy) {
	sendPresence(s_buddy, "", true);
	s_buddy->setOnline();
}

void SpectrumRosterManager::handleBuddySignedOn(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddySignedOn(s_buddy);
}

void SpectrumRosterManager::handleBuddySignedOff(AbstractSpectrumBuddy *s_buddy) {
	sendPresence(s_buddy, "", true);
	s_buddy->setOffline();
}

void SpectrumRosterManager::handleBuddySignedOff(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddySignedOff(s_buddy);
}

void SpectrumRosterManager::handleBuddyStatusChanged(AbstractSpectrumBuddy *s_buddy, PurpleStatus *status, PurpleStatus *old_status) {
	sendPresence(s_buddy, "", true);
	s_buddy->setOnline();
}

void SpectrumRosterManager::handleBuddyStatusChanged(PurpleBuddy *buddy, PurpleStatus *status, PurpleStatus *old_status) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddyStatusChanged(s_buddy, status, old_status);
}

void SpectrumRosterManager::handleBuddyRemoved(AbstractSpectrumBuddy *s_buddy) {
	if (s_buddy->getFlags() & SPECTRUM_BUDDY_IGNORE)
		return;
	m_subscribeCache.erase(s_buddy->getName());
	removeFromLocalRoster(s_buddy->getName());
	removeBuddy(s_buddy);
}

void SpectrumRosterManager::handleBuddyRemoved(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	handleBuddyRemoved(s_buddy);
}

void SpectrumRosterManager::handleBuddyCreated(AbstractSpectrumBuddy *s_buddy) {
	std::string alias = s_buddy->getAlias();
	std::string name = s_buddy->getName();
	if (name.empty()) {
		Log(m_user->jid(), "handleBuddyCreated ERROR! Name is EMPTY! " << name << " ("<< alias <<")");
		return;
	}

	Log(m_user->jid(), "handleBuddyCreated: " << name << " ("<< alias <<")");

	if (!isInRoster(name, "both")) {
		m_syncTimer->start();
		Log(m_user->jid(), "Not in roster => adding to subscribe cache");
		m_subscribeCache[name] = s_buddy;
	}
}

void SpectrumRosterManager::handleBuddyCreated(PurpleBuddy *buddy) {
	if (buddy==NULL || m_loadingFromDB)
		return;
#ifndef TESTS
	buddy->node.ui_data = (void *) new SpectrumBuddy(-1, buddy);
#endif
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	if (!isInRoster(s_buddy->getName())) {
		s_buddy->setFlags(SPECTRUM_BUDDY_JID_ESCAPING);
		handleBuddyCreated(s_buddy);
	}
	else {
		// TODO: PurpleBuddy with this name is already in roster. That means this particular PurpleBuddy represents
		// another group for that contact. We currently handles only one groups per PurpleBuddy, but we could handle
		// that better in the future.
		// SPECTRUM_BUDDY_IGNORE means that this buddy is ignored by RosterStorage class.
		AbstractSpectrumBuddy *s_buddy_1 = getRosterItem(s_buddy->getName());
		s_buddy->setFlags(s_buddy_1->getFlags() | SPECTRUM_BUDDY_IGNORE);
	}
}


bool SpectrumRosterManager::syncBuddiesCallback() {
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

void SpectrumRosterManager::sendNewBuddies() {
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
			if (s_buddy->getSubscription() != "both") {
				std::string jid = s_buddy->getBareJid();
				std::string alias = s_buddy->getAlias();

				s_buddy->setSubscription("ask");
				addRosterItem(s_buddy);

				item = new Tag("item");
				item->addAttribute("action", "add");
				item->addAttribute("jid", jid);
				item->addAttribute("name", alias);
				item->addChild( new Tag("group", s_buddy->getGroup()));
				x->addChild(item);
			}
			it++;
		}
		tag->addChild(x);
		Transport::instance()->send(tag);
	}
	else {
		std::map<std::string, AbstractSpectrumBuddy *>::iterator it = m_subscribeCache.begin();
		while (it != m_subscribeCache.end()) {
			AbstractSpectrumBuddy *s_buddy = (*it).second;
			if (s_buddy->getSubscription() != "both") {
				std::string alias = s_buddy->getAlias();

				Tag *tag = new Tag("presence");
				tag->addAttribute("type", "subscribe");
				tag->addAttribute("from", s_buddy->getBareJid());
				tag->addAttribute("to", m_user->jid());
				if (!alias.empty()) {
					Tag *nick = new Tag("nick", alias);
					nick->addAttribute("xmlns","http://jabber.org/protocol/nick");
					tag->addChild(nick);
				}
				Transport::instance()->send(tag);

				s_buddy->setSubscription("ask");
				addRosterItem(s_buddy);
			}
			it++;
		}
	}

	m_subscribeCache.clear();
	m_subscribeLastCount = -1;
}

void SpectrumRosterManager::handlePresence(const Presence &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag)
		return;

	// Probe presence
	if (stanza.subtype() == Presence::Probe && stanza.to().username() != "") {
		std::string name = purpleUsername(stanza.to().username());
// 		std::for_each( name.begin(), name.end(), replaceJidCharacters() );
		sendPresence(name);
	}
	delete stanzaTag;
}

authRequest *SpectrumRosterManager::handleAuthorizationRequest(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data) {
	std::string name(remote_user);
	
	authRequest *req = new authRequest;
	req->authorize_cb = authorize_cb;
	req->deny_cb = deny_cb;
	req->user_data = user_data;
	req->account = m_user->account();
	req->who = name;
	m_authRequests[name] = req;

	Log(m_user->jid(), "purpleAuthorizeReceived: " << name << "on_list:" << on_list);
// 	std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	name = JID::escapeNode(name);
	// send subscribe presence to user
	Tag *tag = new Tag("presence");
	tag->addAttribute("type", "subscribe" );
	tag->addAttribute("from", name + "@" + Transport::instance()->jid());
	tag->addAttribute("to", m_user->jid());

	if (alias) {
		Tag *nick = new Tag("nick", std::string(alias));
		nick->addAttribute("xmlns","http://jabber.org/protocol/nick");
		tag->addChild(nick);
	}

	Transport::instance()->send(tag);
	return req;
}

void SpectrumRosterManager::removeAuthRequest(const std::string &remote_user) {
	if (m_authRequests.find(remote_user) == m_authRequests.end())
		return;
	m_authRequests.erase(remote_user);
}

bool SpectrumRosterManager::hasAuthRequest(const std::string &remote_user) {
	return m_authRequests.find(remote_user) != m_authRequests.end();
}

void SpectrumRosterManager::handleSubscription(const Subscription &subscription) {
	std::string remote_user = purpleUsername(subscription.to().username());
// 	std::string remote_user(subscription.to().username());
// 	std::for_each( remote_user.begin(), remote_user.end(), replaceJidCharacters() );
	if (remote_user.empty()) {
		Log(m_user->jid(), "handleSubscription: Remote user is EMPTY!");
		return;
	}

	if (m_user->isConnected()) {
		if (subscription.subtype() == Subscription::Subscribed) {
			if (hasAuthRequest(remote_user)) {
				Log(m_user->jid(), "subscribed presence for authRequest arrived => accepting the request");
				m_authRequests[remote_user]->authorize_cb(m_authRequests[remote_user]->user_data);
				removeAuthRequest(remote_user);
			}
			PurpleBuddy *buddy = purple_find_buddy(m_user->account(), remote_user.c_str());
			if (!isInRoster(remote_user, "both")) {
				if (buddy) {
					Log(m_user->jid(), "adding this user to local roster and sending presence");
					if (!isInRoster(remote_user)) {
						// add user to mysql database and to local cache
						addRosterItem(buddy);
						Transport::instance()->sql()->addBuddy(m_user->storageId(), remote_user, "both");
						storeBuddy(buddy);
					}
					else {
						getRosterItem(remote_user)->setSubscription("both");
						Transport::instance()->sql()->updateBuddySubscription(m_user->storageId(), remote_user, "both");
						storeBuddy(buddy);
					}
				} else {
					Log(m_user->jid(), "user is not in legacy network contact lists => nothing to be subscribed");
				}
			}
			if (buddy) {
				// inform user about status
				AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
				sendPresence(s_buddy);
			}
			return;
		}
		else if (subscription.subtype() == Subscription::Subscribe) {
			if (isInRoster(remote_user)) {
// 				SpectrumBuddy *s_buddy = (SpectrumBuddy *) getRosterItem(remote_user);
				Tag *reply = new Tag("presence");
				reply->addAttribute( "to", subscription.from().bare() );
				reply->addAttribute( "from", subscription.to().bare() );
				reply->addAttribute( "type", "subscribe" );
				Transport::instance()->send( reply );

				Log(m_user->jid(), "subscribe presence; user is already in roster => sending subscribed");
				// username is in local roster (he/her is in ICQ roster too),
				// so we should send subscribed presence
				reply = new Tag("presence");
				reply->addAttribute( "to", subscription.from().bare() );
				reply->addAttribute( "from", subscription.to().bare() );
				reply->addAttribute( "type", "subscribed" );
				Transport::instance()->send( reply );
				PurpleBuddy *buddy = purple_find_buddy(m_user->account(), remote_user.c_str());
				if (buddy) {
					Log(m_user->jid(), "Trying to purple_account_add_buddy just to be sure.");
					purple_account_add_buddy(m_user->account(), buddy);
				}
			}
			else {
				Log(m_user->jid(), "subscribe presence; user is not in roster => adding to legacy network");
				// this user is not in local roster, so we have to send add_buddy request
				// to our legacy network and wait for response
				m_loadingFromDB = true;
				PurpleBuddy *buddy = purple_buddy_new(m_user->account(), remote_user.c_str(), remote_user.c_str());
#ifndef TESTS
				SpectrumBuddy *s_buddy = new SpectrumBuddy(-1, buddy);
				buddy->node.ui_data = (void *) s_buddy;
				s_buddy->setSubscription("to");
				if (usesJidEscaping(subscription.to().username()))
					s_buddy->setFlags(s_buddy->getFlags() | SPECTRUM_BUDDY_JID_ESCAPING);
#endif
				purple_blist_add_buddy(buddy, NULL, NULL ,NULL);
				purple_account_add_buddy(m_user->account(), buddy);
				m_loadingFromDB = false;
			}
			return;
		} else if (subscription.subtype() == Subscription::Unsubscribe || subscription.subtype() == Subscription::Unsubscribed) {
			PurpleBuddy *buddy = purple_find_buddy(m_user->account(), remote_user.c_str());
			if (subscription.subtype() == Subscription::Unsubscribed) {
				// user responds to auth request from legacy network and deny it
				if (hasAuthRequest(remote_user)) {
					Log(m_user->jid(), "unsubscribed presence for authRequest arrived => rejecting the request");
					m_authRequests[remote_user]->deny_cb(m_authRequests[remote_user]->user_data);
					removeAuthRequest(remote_user);
					return;
				}
			}
			if (buddy) {
				Log(m_user->jid(), "unsubscribed presence => removing this contact from legacy network");
				long id = 0;
				if (buddy->node.ui_data) {
					AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
					id = s_buddy->getId();
				}
				Transport::instance()->sql()->removeBuddy(m_user->storageId(), remote_user, id);

				purple_account_remove_buddy(m_user->account(), buddy, purple_buddy_get_group(buddy));
				purple_blist_remove_buddy(buddy);
			} else {
				// this contact is not in legacy network buddy list, so there is nothing to remove...
			}
			removeFromLocalRoster(remote_user);

			// inform user about removing this contact
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", subscription.from().bare());
			tag->addAttribute("from", subscription.to().username() + "@" + Transport::instance()->jid() + "/bot");
			if(subscription.subtype() == Subscription::Unsubscribe) {
				tag->addAttribute( "type", "unsubscribe" );
			} else {
				tag->addAttribute( "type", "unsubscribed" );
			}
			Transport::instance()->send( tag );
			return;
		}
	}
	else {
		Log(subscription.from().full(), "Subscribe presence received, but is not connected to legacy network yet.");
	}
}
