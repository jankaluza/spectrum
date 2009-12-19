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

#ifndef TESTS
#include "spectrumbuddy.h"
#endif

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
	m_loadingFromDB = false;
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

	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) g_hash_table_lookup(m_roster, name.c_str());

	if (s_buddy) {
		if (subscription.empty())
			return true;
		return subscription == s_buddy->getSubscription();
	}
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

void RosterManager::loadRoster() {
	m_loadingFromDB = true;
	setRoster(Transport::instance()->sql()->getBuddies(m_user->storageId(), m_user->account()));
	m_loadingFromDB = false;
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
	}
}

void RosterManager::handleBuddyCreated(PurpleBuddy *buddy) {
	if (buddy==NULL || m_loadingFromDB)
		return;
#ifndef TESTS
	buddy->node.ui_data = (void *) new SpectrumBuddy(-1, buddy);
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
	handleBuddyCreated(s_buddy);
#endif
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

			s_buddy->setSubscription("ask");
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
			tag->addAttribute("to", m_user->jid());
			Tag *nick = new Tag("nick", alias);
			nick->addAttribute("xmlns","http://jabber.org/protocol/nick");
			tag->addChild(nick);
			Transport::instance()->send(tag);

			s_buddy->setSubscription("ask");
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

authRequest *RosterManager::handleAuthorizationRequest(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message, gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb, PurpleAccountRequestAuthorizationCb deny_cb, void *user_data) {
	std::string name(remote_user);
	
	authRequest *req = new authRequest;
	req->authorize_cb = authorize_cb;
	req->deny_cb = deny_cb;
	req->user_data = user_data;
	req->account = m_user->account();
	req->who = name;
	m_authRequests[name] = req;

	Log(m_user->jid(), "purpleAuthorizeReceived: " << name << "on_list:" << on_list);
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

void RosterManager::removeAuthRequest(const std::string &remote_user) {
	if (m_authRequests.find(remote_user) == m_authRequests.end())
		return;
	delete m_authRequests[remote_user];
	m_authRequests.erase(remote_user);
}

bool RosterManager::hasAuthRequest(const std::string &remote_user) {
	return m_authRequests.find(remote_user) != m_authRequests.end();
}

void RosterManager::handleSubscription(const Subscription &subscription) {
	std::string remote_user(subscription.to().username());
	std::for_each( remote_user.begin(), remote_user.end(), replaceJidCharacters() );

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
					if (!isInRoster(subscription.to().username())) {
						// add user to mysql database and to local cache
						addRosterItem(buddy);
						p->sql()->addBuddy(m_userID, (std::string) subscription.to().username(), "both");
					}
					else {
						getRosterItem(subscription.to().username())->setSubscription("both");
						p->sql()->updateBuddySubscription(m_userID, (std::string) subscription.to().username(), "both");
					}
					// user is in ICQ contact list so we can inform jabber user
					// about status
					SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
					Tag *tag = s_buddy->generatePresenceStanza(m_features);
					if (tag) {
						tag->addAttribute("to", m_jid);
						p->j->send(tag);
					}
				} else {
					Log(m_user->jid(), "user is not in legacy network contact lists => nothing to be subscribed");
					// this user is not in ICQ contact list... It's nothing to do here,
					// because there is no user to authorize
				}
			}
			return;
		}
		else if (subscription.subtype() == Subscription::Subscribe) {
			std::string name(subscription.to().username());
			std::for_each( name.begin(), name.end(), replaceJidCharacters() );
			if (isInRoster(subscription.to().username(), "")) {
				SpectrumBuddy *s_buddy = (SpectrumBuddy *) getRosterItem(subscription.to().username());
				if (s_buddy->getSubscription() == "ask") {
					Tag *reply = new Tag("presence");
					reply->addAttribute( "to", subscription.from().bare() );
					reply->addAttribute( "from", subscription.to().bare() );
					reply->addAttribute( "type", "subscribe" );
					p->j->send( reply );
				}
				Log(m_jid, "subscribe presence; user is already in roster => sending subscribed");
				// username is in local roster (he/her is in ICQ roster too),
				// so we should send subscribed presence
				Tag *reply = new Tag("presence");
				reply->addAttribute( "to", subscription.from().bare() );
				reply->addAttribute( "from", subscription.to().bare() );
				reply->addAttribute( "type", "subscribed" );
				p->j->send( reply );
			}
			else {
				Log(m_jid, "subscribe presence; user is not in roster => adding to legacy network");
				// this user is not in local roster, so we have to send add_buddy request
				// to our legacy network and wait for response
				std::string name(subscription.to().username());
				std::for_each( name.begin(), name.end(), replaceJidCharacters() );
				PurpleBuddy *buddy = purple_buddy_new(m_account, name.c_str(), name.c_str());
				purple_blist_add_buddy(buddy, NULL, NULL ,NULL);
				purple_account_add_buddy(m_account, buddy);
			}
			return;
		} else if(subscription.subtype() == Subscription::Unsubscribe || subscription.subtype() == Subscription::Unsubscribed) {
			std::string name(subscription.to().username());
			std::for_each( name.begin(), name.end(), replaceJidCharacters() );
			PurpleBuddy *buddy = purple_find_buddy(m_account, name.c_str());
			if (subscription.subtype() == Subscription::Unsubscribed) {
				// user respond to auth request from legacy network and deny it
				if (hasAuthRequest((std::string) subscription.to().username())) {
					Log(m_jid, "unsubscribed presence for authRequest arrived => rejecting the request");
					m_authRequests[(std::string) subscription.to().username()].deny_cb(m_authRequests[(std::string) subscription.to().username()].user_data);
					m_authRequests.erase(subscription.to().username());
					return;
				}
			}
			if (buddy) {
				Log(m_jid, "unsubscribed presence => removing this contact from legacy network");
				long id = 0;
				if (buddy->node.ui_data) {
					SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
					id = s_buddy->getId();
				}
				p->sql()->removeBuddy(m_userID, subscription.to().username(), id);
				// thi contact is in ICQ contact list, so we can remove him/her
				purple_account_remove_buddy(m_account, buddy, purple_buddy_get_group(buddy));
				purple_blist_remove_buddy(buddy);
			} else {
				// this contact is not in ICQ contact list, so there is nothing to remove...
			}
			removeFromLocalRoster(subscription.to().username());

			// inform user about removing this contact
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", subscription.from().bare());
			tag->addAttribute("from", subscription.to().username() + "@" + p->jid() + "/bot");
			if(subscription.subtype() == Subscription::Unsubscribe) {
				tag->addAttribute( "type", "unsubscribe" );
			} else {
				tag->addAttribute( "type", "unsubscribed" );
			}
			p->j->send( tag );
			return;
		}
	}
}