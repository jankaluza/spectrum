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
#include "main.h"
#include "log.h"
#include "striphtmltags.h"
#include "transport.h"

AbstractSpectrumBuddy::AbstractSpectrumBuddy(long id) : m_id(id), m_online(false), m_subscription("both") {
}

AbstractSpectrumBuddy::~AbstractSpectrumBuddy() {
}

long AbstractSpectrumBuddy::getId() {
	return m_id;
}

void AbstractSpectrumBuddy::setId(long id) {
	m_id = id;
}

std::string AbstractSpectrumBuddy::getSafeName() {
	std::string name = getName();
	std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	return name;
}

std::string AbstractSpectrumBuddy::getJid() {
	return getSafeName() + "@" + Transport::instance()->jid() + "/bot";
}


bool AbstractSpectrumBuddy::isOnline() {
	return m_online;
}

void AbstractSpectrumBuddy::setOnline() {
	m_online = true;
}

void AbstractSpectrumBuddy::setOffline() {
	m_online = false;
}

void AbstractSpectrumBuddy::setSubscription(const std::string &subscription) {
	m_subscription = subscription;
}

const std::string &AbstractSpectrumBuddy::getSubscription() {
	return m_subscription;
}

