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
#include "user.h"

#ifndef TESTS
#include "spectrumbuddy.h"
#endif

struct SendPresenceToAllData {
	int features;
	int user_feature;
	std::string to;
	bool markOffline;
};

static void merge_buddy(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SpectrumRosterManager *manager = (SpectrumRosterManager *) data;

	manager->mergeBuddy(s_buddy);
}

static void handleAskSubscriptionBuddies(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SpectrumRosterManager *manager = (SpectrumRosterManager *) data;

	if (s_buddy->getSubscription() == "ask") {
		manager->handleBuddyCreated(s_buddy);
	}
}

static void sendUnavailablePresence(gpointer key, gpointer v, gpointer data) {
	Log("sendUnavailablePresence", (char *) key);
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SendPresenceToAllData *d = (SendPresenceToAllData *) data;
	std::string &to = d->to;

	if (s_buddy->isOnline()) {
		SpectrumRosterManager::sendPresence(s_buddy->getJid(), to, "unavailable");
		if (d->markOffline)
			s_buddy->setOffline();
	}
}

static void sendCurrentPresence(gpointer key, gpointer v, gpointer data) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	SendPresenceToAllData *d = (SendPresenceToAllData *) data;
	int features = d->features;
	int user_feature = d->user_feature;
	std::string &to = d->to;
	std::cout << "online: " << s_buddy->isOnline() << "\n";
	if (s_buddy->isOnline()) {
		Tag *tag = s_buddy->generatePresenceStanza(features);
		std::cout << "online: " << tag << "\n";
		if (tag) {
			tag->addAttribute("to", to);
			Transport::instance()->send( tag );
		}
		if (features & TRANSPORT_FEATURE_XSTATUS) {
			tag = s_buddy->generateXStatusStanza(user_feature);
			if (tag) {
				tag->addAttribute("to", JID(to).bare());
				Transport::instance()->send(tag);
			}
		}
	}
}

static gboolean sync_cb(gpointer data) {
	SpectrumRosterManager *manager = (SpectrumRosterManager *) data;
	return manager->syncBuddiesCallback();
}

static gboolean sendRosterPresences(gpointer data) {
	SpectrumRosterManager *manager = (SpectrumRosterManager *) data;
	return manager->_sendRosterPresences();
}

SpectrumRosterManager::SpectrumRosterManager(User *user) : RosterStorage(user) {
	m_user = user;
	m_syncTimer = new SpectrumTimer(CONFIG().protocol == "icq" ? 12000 : 1000, &sync_cb, this);
	m_presenceTimer = new SpectrumTimer(5000, &sendRosterPresences, this);
	m_subscribeLastCount = -1;
	m_roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	m_loadingFromDB = false;
	m_rosterPushesContext = 0;
	m_supportRosterIQ = false;
}

SpectrumRosterManager::~SpectrumRosterManager() {
	g_hash_table_destroy(m_roster);
	delete m_syncTimer;
	delete m_presenceTimer;
}

bool SpectrumRosterManager::isInRoster(const std::string &name, const std::string &subscription) {
	std::string preparedUin(name);
	Transport::instance()->protocol()->prepareUsername(preparedUin, m_user->account());
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) g_hash_table_lookup(m_roster, preparedUin.c_str());

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
	data->user_feature = m_user->getMergedFeatures();
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
	std::string preparedUin(uin);
	Transport::instance()->protocol()->prepareUsername(preparedUin, m_user->account());
	m_subscribeCache.erase(uin);
	if (!g_hash_table_lookup(m_roster, preparedUin.c_str()))
		return;
	g_hash_table_remove(m_roster, preparedUin.c_str());
}

AbstractSpectrumBuddy *SpectrumRosterManager::getRosterItem(const std::string &uin) {
	std::string preparedUin(uin);
	Transport::instance()->protocol()->prepareUsername(preparedUin, m_user->account());
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) g_hash_table_lookup(m_roster, preparedUin.c_str());
	return s_buddy;
}

void SpectrumRosterManager::addRosterItem(AbstractSpectrumBuddy *s_buddy) {
	if (isInRoster(s_buddy->getName()))
		return;
	std::string preparedUin(s_buddy->getName());
	Transport::instance()->protocol()->prepareUsername(preparedUin, m_user->account());
	g_hash_table_replace(m_roster, g_strdup(preparedUin.c_str()), s_buddy);
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

	if (m_user->getFeatures() & TRANSPORT_FEATURE_XSTATUS) {
		tag = s_buddy->generateXStatusStanza(m_user->getMergedFeatures());
		if (tag) {
			tag->addAttribute("to", m_user->jid());
			Transport::instance()->send(tag);
		}
	}
}

void SpectrumRosterManager::sendPresence(const std::string &name, const std::string &resource) {
	AbstractSpectrumBuddy *s_buddy = getRosterItem(name);
	if (s_buddy) {
		sendPresence(s_buddy, resource);
	}
	else {
		Log(m_user->jid(), "answering to probe presence with unavailable presence");
		std::string n(name);
		std::for_each( n.begin(), n.end(), replaceBadJidCharacters() );
		std::string from = n + "@" + Transport::instance()->jid();
		std::string to = m_user->jid() + std::string(resource.empty() ? "" : "/" + resource);
		SpectrumRosterManager::sendPresence(from, to, "unavailable");
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
	if (s_buddy->getFlags() & SPECTRUM_BUDDY_IGNORE) {
		// just for case we would remove some PurpleBuddy* which is used
		// in SpectrumBuddy.
		AbstractSpectrumBuddy *s_buddy_1 = getRosterItem(s_buddy->getName());
		// this should be always true, but it's not bad to be defensive here.
		if (s_buddy_1 && s_buddy_1 != s_buddy) {
			s_buddy_1->handleBuddyRemoved(s_buddy->getBuddy());
			// this should be always false, but if it's not, we have to remove
			// this buddy from local roster
			if (s_buddy_1->getBuddy() != s_buddy->getBuddy()) {
				return;
			}
		}
	}
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
	else {
		if (m_supportRosterIQ && m_xmppRoster.find(name) != m_xmppRoster.end()) {
			// first synchronization = From XMPP to legacy network and we don't care what's on legacy network
			if (m_user->getSetting<bool>("first_synchronization_done") == false) {
				s_buddy->changeAlias(m_xmppRoster[name].nickname);
				s_buddy->changeGroup(m_xmppRoster[name].groups);
			}
			// other synchronizations are done from ICQ to XMPP here
			else {
				m_syncTimer->start();
				m_subscribeCache[name] = s_buddy;
			}
		}
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
		if (Transport::instance()->getConfiguration().jid_escaping)
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

	if (m_supportRosterIQ) {
		std::map<std::string, AbstractSpectrumBuddy *>::iterator it = m_subscribeCache.begin();
		while (it != m_subscribeCache.end()) {
			AbstractSpectrumBuddy *s_buddy = (*it).second;
			std::string jid = s_buddy->getBareJid();
			std::string alias = s_buddy->getAlias();
			std::string name = s_buddy->getName();

			// check if buddy in jabber roster differs from buddy in legacy network contact list.
			bool differs = true;
			if (m_xmppRoster.find(name) != m_xmppRoster.end()) {
				std::string group = m_xmppRoster[name].groups.size() == 0 ? "Buddies" : m_xmppRoster[name].groups.front();
				differs = m_xmppRoster[name].nickname != alias ||  s_buddy->getGroup() != group;
			}

			// send roster push if buddies are different or subscription is not both
			if (differs || s_buddy->getSubscription() != "both") {
// 				std::string group = m_xmppRoster[name].groups.size() == 0 ? "Buddies" : m_xmppRoster[name].groups.front();
// 				std::cout << s_buddy->getSubscription() << " " << group << " " << s_buddy->getGroup() << " " << m_xmppRoster[name].nickname <<  " " << alias << "\n";
				m_rosterPushes[m_rosterPushesContext++] = s_buddy;
				SpectrumRosterManager::sendRosterPush(m_user->jid(), jid, "both", alias, s_buddy->getGroup(), this, m_rosterPushesContext);

				// Set subscription to both and store buddy
				s_buddy->setSubscription("both");
				storeBuddy(s_buddy);
				addRosterItem(s_buddy);
			}
			it++;
		}
	}
	else {
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
					SpectrumRosterManager::sendSubscribePresence(s_buddy->getBareJid(), m_user->jid(), alias);

					s_buddy->setSubscription("ask");
					addRosterItem(s_buddy);
				}
				it++;
			}
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

	if (m_user->getSetting<bool>("reject_authorizations") && getRosterItem(remote_user) == NULL) {
		Log(m_user->jid(), "purpleAuthorization rejected: " << name << " on_list:" << on_list);
		deny_cb(user_data);
		return NULL;
	}
		

	authRequest *req = new authRequest;
	req->authorize_cb = authorize_cb;
	req->deny_cb = deny_cb;
	req->user_data = user_data;
	req->account = m_user->account();
	req->who = name;
	req->mainJID = m_user->jid();
	m_authRequests[name] = req;

	Log(m_user->jid(), "purpleAuthorizeReceived: " << name << "on_list:" << on_list);

	if (Transport::instance()->getConfiguration().jid_escaping) {
		name = JID::escapeNode(name);
	}
	else {
		std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	}
	// send subscribe presence to user
	std::string nickname = alias ? alias : "";
	SpectrumRosterManager::sendSubscribePresence(name + "@" + Transport::instance()->jid(), m_user->jid(), nickname);
	return req;
}

void SpectrumRosterManager::removeAuthRequest(const std::string &remote_user) {
	m_authRequests.erase(remote_user);
	if (Transport::instance()->getConfiguration().jid_escaping) {
		std::string name(remote_user);
		JID::escapeNode(name);
		m_authRequests.erase(name);
	}
}

bool SpectrumRosterManager::hasAuthRequest(const std::string &remote_user) {
	return m_authRequests.find(remote_user) != m_authRequests.end();
}

void SpectrumRosterManager::handleSubscription(const Subscription &subscription) {
	std::string remote_user = purpleUsername(subscription.to().username());
	if (remote_user.empty()) {
		Log(m_user->jid(), "handleSubscription: Remote user is EMPTY!");
		return;
	}

	if (m_user->isConnected()) {
		PurpleBuddy *buddy = NULL;

		// Try to get PurpleBuddy from SpectrumBuddy at first, because SpectrumBuddy
		// uses normalized username (buddy->name could be "User", but remote_user is normalized
		// so it's "user" for example).
		AbstractSpectrumBuddy *s_buddy = getRosterItem(remote_user);
		if (s_buddy)
			buddy = s_buddy->getBuddy();
		else
			buddy = purple_find_buddy(m_user->account(), remote_user.c_str());

		if (subscription.subtype() == Subscription::Subscribed) {
			// Accept authorize request.
			if (hasAuthRequest(remote_user)) {
				Log(m_user->jid(), "subscribed presence for authRequest arrived => accepting the request");
				m_authRequests[remote_user]->authorize_cb(m_authRequests[remote_user]->user_data);
				removeAuthRequest(remote_user);
			}

			if (!isInRoster(remote_user, "both")) {
				if (buddy) {
					Log(m_user->jid(), "adding this user to local roster and sending presence");
					if (s_buddy == NULL) {
						// There is PurpleBuddy, but not SpectrumBuddy (that should not happen, but this
						// code doesn't break anything).
						addRosterItem(buddy);
						Transport::instance()->sql()->addBuddy(m_user->storageId(), remote_user, "both");
						storeBuddy(buddy);
					}
					else {
						// Update subscription.
						s_buddy->setSubscription("both");
						Transport::instance()->sql()->updateBuddySubscription(m_user->storageId(), remote_user, "both");
						storeBuddy(buddy);
					}
				} else {
					Log(m_user->jid(), "user is not in legacy network contact lists => nothing to be subscribed");
				}
			}

			// XMPP user is able to get the first status presence just after "subscribed" presence.
			if (s_buddy) {
				sendPresence(s_buddy);
			}
			return;
		}
		else if (subscription.subtype() == Subscription::Subscribe) {
			// User is in roster, so that's probably response for RIE.
			if (s_buddy) {
				Log(m_user->jid(), "subscribe presence; user is already in roster => sending subscribed");

				s_buddy->setSubscription("to");
				SpectrumRosterManager::sendPresence(subscription.to().bare(), subscription.from().bare(), "subscribe");
				SpectrumRosterManager::sendPresence(subscription.to().bare(), subscription.from().bare(), "subscribed");

				// Sometimes there is "subscribed" request for new user received before "subscribe",
				// so s_buddy could be already there, but purple_account_add_buddy has not been called.
				if (buddy) {
					Log(m_user->jid(), "Trying to purple_account_add_buddy just to be sure.");
					purple_account_add_buddy(m_user->account(), buddy);
				}
			}
			// User is not in roster, so that's probably new subscribe request.
			else {
				Log(m_user->jid(), "subscribe presence; user is not in roster => adding to legacy network");
				// Disable handling new-buddy event, because we're going to create new buddy by Spectrum, not
				// by libpurple (so we don't want to add it to RIE queue and send back to XMPP user).
				m_loadingFromDB = true;
				PurpleBuddy *buddy = purple_buddy_new(m_user->account(), remote_user.c_str(), remote_user.c_str());
#ifndef TESTS
				SpectrumBuddy *s_buddy = new SpectrumBuddy(-1, buddy);
				buddy->node.ui_data = (void *) s_buddy;
				s_buddy->setSubscription("to");
				if (usesJidEscaping(subscription.to().username()))
					s_buddy->setFlags(s_buddy->getFlags() | SPECTRUM_BUDDY_JID_ESCAPING);
				addRosterItem(s_buddy);
#endif
				// Add newly created buddy to legacy network roster.
				purple_blist_add_buddy(buddy, NULL, NULL ,NULL);
				purple_account_add_buddy(m_user->account(), buddy);
				m_loadingFromDB = false;
				SpectrumRosterManager::sendPresence(subscription.to().bare(), subscription.from().bare(), "subscribe");
				SpectrumRosterManager::sendPresence(subscription.to().bare(), subscription.from().bare(), "subscribed");
			}
			return;
		} else if (subscription.subtype() == Subscription::Unsubscribe || subscription.subtype() == Subscription::Unsubscribed) {
			if (subscription.subtype() == Subscription::Unsubscribed) {
				// Reject authorize request.
				if (hasAuthRequest(remote_user)) {
					Log(m_user->jid(), "unsubscribed presence for authRequest arrived => rejecting the request");
					m_authRequests[remote_user]->deny_cb(m_authRequests[remote_user]->user_data);
					removeAuthRequest(remote_user);
					return;
				}
			}

			// Buddy is in legacy network roster, so we can remove it.
			if (buddy) {
				Log(m_user->jid(), "unsubscribed presence => removing this contact from legacy network");
				// Remove buddy from database, if he's already there.
				if (s_buddy) {
					long id = s_buddy->getId();
					Transport::instance()->sql()->removeBuddy(m_user->storageId(), remote_user, id);
				}

				// Remove buddy from legacy network roster.
				purple_account_remove_buddy(m_user->account(), buddy, purple_buddy_get_group(buddy));
				purple_blist_remove_buddy(buddy);
			} else {
				// this buddy is not in legacy network roster, so there is nothing to remove...
				Transport::instance()->sql()->removeBuddy(m_user->storageId(), remote_user, 0);
			}

			removeFromLocalRoster(remote_user);

			// inform user about removing this buddy
			SpectrumRosterManager::sendPresence(subscription.to().username() + "@" + Transport::instance()->jid() + "/bot",
												subscription.from().bare(),
												subscription.subtype() == Subscription::Unsubscribe ? "unsubscribe" : "unsubscribed");
			return;
		}
	}
	else {
		Log(subscription.from().full(), "Subscribe presence received, but is not connected to legacy network yet.");
	}
}

void SpectrumRosterManager::handleRosterResponse(Tag *iq) {
	if (iq->findAttribute("type") == "error") {
		Log(m_user->jid(), "This server doesn't support remoter-roster XEP");
		return;
	}

	if (m_supportRosterIQ == false) {
		Log(m_user->jid(), "This server supports remoter-roster XEP");
		m_supportRosterIQ = true;
	}

	if (iq->findAttribute("type") == "set") {
		Tag *query = iq->findChild("query");
		if (query == NULL) {
			std::cout << "query is null!\n";
			return;
		}
		Tag *item = query->findChild("item");
		if (item == NULL) {
			std::cout << "item is null!\n";
			return;
		}

		// ignore buddy removing
		if (item->findAttribute("subscription") == "remove") {
			return;
		}

		std::string remote_user = purpleUsername(JID(item->findAttribute("jid")).username());
		AbstractSpectrumBuddy *s_buddy = getRosterItem(remote_user);
		if (s_buddy == NULL) {
			std::cout << remote_user << " buddy is null!\n";
			return;
		}

		// Change buddy alias if it changed
		s_buddy->changeAlias(item->findAttribute("name"));

		// Change groups if they changed
		std::list<std::string> groups;
		std::list<Tag*> tags = item->findChildren("group");
		for (std::list<Tag*>::const_iterator it = tags.begin(); it != tags.end(); it++) {
			groups.push_back((*it)->cdata());
		}
		s_buddy->changeGroup(groups);
		storeBuddy(s_buddy);
	}
	else if (iq->findAttribute("type") == "result") {
		Tag *query = iq->findChild("query");
		if (query == NULL) {
			std::cout << "query is null!\n";
			return;
		}

		std::list<Tag*> items = query->findChildren("item");
		for (std::list<Tag*>::const_iterator it = items.begin(); it != items.end(); it++) {
			Tag *item = *it;
			std::string remote_user = purpleUsername(JID(item->findAttribute("jid")).username());
			m_xmppRoster[remote_user].jid = item->findAttribute("jid");
			m_xmppRoster[remote_user].nickname = item->findAttribute("name");
			std::list<Tag*> tags = item->findChildren("group");
			for (std::list<Tag*>::const_iterator it = tags.begin(); it != tags.end(); it++) {
				m_xmppRoster[remote_user].groups.push_back((*it)->cdata());
			}
		}
	}
}

void SpectrumRosterManager::handleIqID(const IQ &iq, int id) {
	std::cout << "rosterPush with id " << id << "\n";
	// This buddy was added into roster by transport just now, so we have to
	// send initial presence.
	// We can send it from here, because ejabberd is not ready just after this
	// result iq... :/ (therefore there's timer)
	if (m_rosterPushes.find(id) != m_rosterPushes.end()) {
		std::cout << "sending presence\n";
		m_presenceTimer->start();
	}
}

bool SpectrumRosterManager::_sendRosterPresences() {
	for (std::map<int, AbstractSpectrumBuddy *>::iterator it = m_rosterPushes.begin(); it != m_rosterPushes.end(); it++) {
		sendPresence((*it).second);
	}
	m_rosterPushes.clear();
	return false;
}

void SpectrumRosterManager::mergeRoster() {
	if (m_supportRosterIQ)
		g_hash_table_foreach(m_roster, merge_buddy, this);
	if (m_user->getSetting<bool>("first_synchronization_done", false) == false) {
		m_user->updateSetting("first_synchronization_done", true);
	}
}

void SpectrumRosterManager::mergeBuddy(AbstractSpectrumBuddy *s_buddy) {
	std::string name = s_buddy->getName();
	if (m_xmppRoster.find(name) != m_xmppRoster.end()) {
		// first synchronization = From XMPP to legacy network and we don't care what's on legacy network
		if (m_user->getSetting<bool>("first_synchronization_done") == false) {
			s_buddy->changeAlias(m_xmppRoster[name].nickname);
			s_buddy->changeGroup(m_xmppRoster[name].groups);
		}
		// other synchronizations are done from ICQ to XMPP here
		else {
			m_syncTimer->start();
			m_subscribeCache[name] = s_buddy;
		}
	}
}

void SpectrumRosterManager::sendRosterPush(const std::string &to, const std::string &jid, const std::string &subscription, const std::string &alias, const std::string &group, IqHandler *ih, int context) {
	IQ iq(IQ::Set, to);
	iq.setFrom(Transport::instance()->jid());

	Tag *x = new Tag("query");
	x->addAttribute("xmlns", "jabber:iq:roster");

	Tag *item = new Tag("item");
	item->addAttribute("jid", jid);
	item->addAttribute("subscription", subscription);
	if (!alias.empty())
		item->addAttribute("name", alias);
	if (!group.empty())
		item->addChild(new Tag("group", group));
	x->addChild(item);
	
	iq.addExtension(new RosterExtension(x));
	delete x;
	if (ih)
		Transport::instance()->send(iq, ih, context);
	else
		Transport::instance()->send(iq.tag());
}

void SpectrumRosterManager::sendPresence(const std::string &from, const std::string &to, const std::string &type, const std::string &message) {
	Tag *tag = new Tag("presence");
	tag->addAttribute("to", to);
	tag->addAttribute("type", type);
	tag->addAttribute("from", from);
	if (!message.empty())
		tag->addChild( new Tag("status", message) );
	Transport::instance()->send(tag);
}

void SpectrumRosterManager::sendPresence(const std::string &from, const std::string &to, const Presence::PresenceType &type, const std::string &message) {
	Presence tag(type, to, message);
	tag.setFrom(from);
	Transport::instance()->send(tag.tag());
}

void SpectrumRosterManager::sendSubscribePresence(const std::string &from, const std::string &to, const std::string &nick) {
	Tag *tag = new Tag("presence");
	tag->addAttribute("type", "subscribe");
	tag->addAttribute("from", from);
	tag->addAttribute("to", to);
	if (!nick.empty()) {
		Tag *n = new Tag("nick", nick);
		n->addAttribute("xmlns", "http://jabber.org/protocol/nick");
		tag->addChild(n);
	}
	Transport::instance()->send(tag);
}

void SpectrumRosterManager::sendRosterGet(const std::string &to) {
	Tag *iq = new Tag("iq");
	iq->addAttribute("from", Transport::instance()->jid());
	iq->addAttribute("to", to);
	iq->addAttribute("id", Transport::instance()->getId());
	iq->addAttribute("type", "get");
	Tag *query = new Tag("query");
	query->addAttribute("xmlns", "jabber:iq:roster");
	iq->addChild(query);
	Transport::instance()->send(iq);
}
