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

#include "abstractspectrumbuddy.h"
#include "main.h" // just for replaceBadJidCharacters
#include "transport.h"
#include "usermanager.h"

AbstractSpectrumBuddy::AbstractSpectrumBuddy(long id) : m_id(id), m_online(false), m_subscription("ask"), m_flags(0) {
}

AbstractSpectrumBuddy::~AbstractSpectrumBuddy() {
}

void AbstractSpectrumBuddy::setId(long id) {
	m_id = id;
}

long AbstractSpectrumBuddy::getId() {
	return m_id;
}

void AbstractSpectrumBuddy::setFlags(int flags) {
	m_flags = flags;
}

int AbstractSpectrumBuddy::getFlags() {
	return m_flags;
}

std::string AbstractSpectrumBuddy::getBareJid() {
	return getSafeName() + "@" + Transport::instance()->jid();
}

std::string AbstractSpectrumBuddy::getJid() {
	return getSafeName() + "@" + Transport::instance()->jid() + "/bot";
}

void AbstractSpectrumBuddy::setOnline() {
#ifndef TESTS
	if (!m_online)
		Transport::instance()->userManager()->buddyOnline();
#endif
	m_online = true;
}

void AbstractSpectrumBuddy::setOffline() {
#ifndef TESTS
	if (m_online)
		Transport::instance()->userManager()->buddyOffline();
#endif
	m_online = false;
	m_lastPresence = Swift::Presence::ref();
}

bool AbstractSpectrumBuddy::isOnline() {
	return m_online;
}

void AbstractSpectrumBuddy::setSubscription(const std::string &subscription) {
	m_subscription = subscription;
}

const std::string &AbstractSpectrumBuddy::getSubscription() {
	return m_subscription;
}

Swift::Presence::ref AbstractSpectrumBuddy::generatePresenceStanza(int features, bool only_new) {
	std::string alias = getAlias();
	std::string name = getSafeName();

	PurpleStatusPrimitive s;
	std::string statusMessage;
	if (!getStatus(s, statusMessage))
		return Swift::Presence::ref();

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setFrom(Swift::JID(getJid()));
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	switch(s) {
		case PURPLE_STATUS_AVAILABLE: {
			break;
		}
		case PURPLE_STATUS_AWAY: {
			presence->setShow(Swift::StatusShow::Away);
			break;
		}
		case PURPLE_STATUS_UNAVAILABLE: {
			presence->setShow(Swift::StatusShow::DND);
			break;
		}
		case PURPLE_STATUS_EXTENDED_AWAY: {
			presence->setShow(Swift::StatusShow::XA);
			break;
		}
		case PURPLE_STATUS_OFFLINE: {
			presence->setType(Swift::Presence::Unavailable);
			break;
		}
		default:
			break;
	}

	if (s != PURPLE_STATUS_OFFLINE) {
		// caps
		presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::CapsInfo (CONFIG().caps)));

		if (features & TRANSPORT_FEATURE_AVATARS) {
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (getIconHash())));
		}
	}

	if (only_new) {
		if (m_lastPresence)
			m_lastPresence->setTo(Swift::JID(""));
		if (m_lastPresence == presence) {
			return Swift::Presence::ref();
		}
		m_lastPresence = presence;
	}

	return presence;
}
