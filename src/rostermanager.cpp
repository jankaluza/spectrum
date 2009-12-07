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
#include "spectrumbuddy.h"
#include "main.h"
#include "user.h"
#include "log.h"
#include "sql.h"
#include "usermanager.h"
#include "caps.h"
#include "spectrumtimer.h"

RosterRow DummyRosterRow;

RosterManager::RosterManager(User *user) {
	m_user = user;
}

RosterManager::~RosterManager() {
	m_roster.clear();
}

/*
 * Returns true if user with UIN `name` and subscription `subscription` is
 * in roster. If subscription.empty(), returns true on any subsciption.
 */
bool RosterManager::isInRoster(const std::string &name,const std::string &subscription) {
	std::map<std::string,RosterRow>::iterator iter = m_roster.begin();
	iter = m_roster.find(name);
	if (iter != m_roster.end()) {
		if (subscription.empty())
			return true;
		if (m_roster[name].subscription == subscription)
			return true;
	}
	return false;
}

/*
 * Generates presence stanza for PurpleBuddy `buddy`. This will
 * create whole <presence> stanza without 'to' attribute.
 */
Tag *RosterManager::generatePresenceStanza(PurpleBuddy *buddy) {
	if (buddy == NULL)
		return NULL;
	GlooxMessageHandler *p = GlooxMessageHandler::instance();
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;

	std::string alias = s_buddy->getAlias();
	std::string name = s_buddy->getSafeName();

	int s;
	std::string statusMessage;
	if (!s_buddy->getStatus(s, statusMessage))
		return NULL;

	Log(m_user->jid(), "Generating presence stanza for user " << name);
	Tag *tag = new Tag("presence");
	tag->addAttribute("from", name + "@" + p->jid() + "/bot");

	if (!statusMessage.empty())
		tag->addChild( new Tag("status", statusMessage) );

	switch(s) {
		case PURPLE_STATUS_AVAILABLE: {
			break;
		}
		case PURPLE_STATUS_AWAY: {
			tag->addChild( new Tag("show", "away" ) );
			break;
		}
		case PURPLE_STATUS_UNAVAILABLE: {
			tag->addChild( new Tag("show", "dnd" ) );
			break;
		}
		case PURPLE_STATUS_EXTENDED_AWAY: {
			tag->addChild( new Tag("show", "xa" ) );
			break;
		}
		case PURPLE_STATUS_OFFLINE: {
			tag->addAttribute( "type", "unavailable" );
			break;
		}
	}

	// caps
	Tag *c = new Tag("c");
	c->addAttribute("xmlns", "http://jabber.org/protocol/caps");
	c->addAttribute("hash", "sha-1");
	c->addAttribute("node", "http://spectrum.im/transport");
	c->addAttribute("ver", p->configuration().hash);
	tag->addChild(c);

	if (m_user->hasTransportFeature(TRANSPORT_FEATURE_AVATARS)) {
		// vcard-temp:x:update
		char *avatarHash = NULL;
		PurpleBuddyIcon *icon = purple_buddy_icons_find(m_user->account(), purple_buddy_get_name(buddy));
		if (icon != NULL) {
			avatarHash = purple_buddy_icon_get_full_path(icon);
			Log(m_user->jid(), "avatarHash");
		}

		if (purple_value_get_boolean(m_user->getSetting("enable_avatars"))) {
			Tag *x = new Tag("x");
			x->addAttribute("xmlns","vcard-temp:x:update");
			if (avatarHash != NULL) {
				Log(m_user->jid(), "Got avatar hash");
				// Check if it's patched libpurple which saves icons to directories
				char *hash = strrchr(avatarHash,'/');
				std::string h;
				if (hash) {
					char *dot;
					hash++;
					dot = strchr(hash, '.');
					if (dot)
						*dot = '\0';
					x->addChild(new Tag("photo", (std::string) hash));
				}
				else
					x->addChild(new Tag("photo", (std::string) avatarHash));
			}
			else {
				Log(m_user->jid(), "no avatar hash");
				x->addChild(new Tag("photo"));
			}
			tag->addChild(x);
		}
		
		if (avatarHash)
			g_free(avatarHash);
	}

	return tag;
}

void RosterManager::sendUnavailablePresenceToAll() {
	Tag *tag;
	for (std::map<std::string, RosterRow>::iterator u = m_roster.begin(); u != m_roster.end() ; u++) {
		if ((*u).second.online){
			std::string name((*u).first);
			std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
			tag = new Tag("presence");
			tag->addAttribute( "to", m_user->jid() );
			tag->addAttribute( "type", "unavailable" );
			tag->addAttribute( "from", name + "@" + GlooxMessageHandler::instance()->jid() + "/bot");
			GlooxMessageHandler::instance()->j->send( tag );
			GlooxMessageHandler::instance()->userManager()->buddyOffline();
			(*u).second.online = false;
		}
	}
}

void RosterManager::sendPresenceToAll(const std::string &to) {
	for (std::map<std::string, RosterRow>::iterator u = m_roster.begin(); u != m_roster.end() ; u++) {
		if ((*u).second.online){
			std::string name((*u).second.uin);
			std::for_each( name.begin(), name.end(), replaceJidCharacters() );
			PurpleBuddy *buddy = purple_find_buddy(m_user->account(), name.c_str());
			if (buddy) {
				Tag *probe = generatePresenceStanza(buddy);
				if (probe) {
					probe->addAttribute("to", to);
					GlooxMessageHandler::instance()->j->send(probe);
				}
			}
		}
	}
}

void RosterManager::removeFromLocalRoster(const std::string &uin) {
	if (isInRoster(uin,"")) {
		// this contact is in our local roster, so we have to remove her/him
		Log(m_user->jid(), "removing this contact from local roster");
		m_roster.erase(uin);
	}
}

RosterRow &RosterManager::getRosterItem(const std::string &uin) {
	if (isInRoster(uin, ""))
		return m_roster[uin];
	return DummyRosterRow;
}

void RosterManager::addRosterItem(RosterRow item) {
	m_roster[item.uin] = item;
}
void RosterManager::loadBuddies() {
	m_roster = GlooxMessageHandler::instance()->sql()->getBuddies(m_user->storageId(), m_user->account());
}

