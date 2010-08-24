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

#include "rosterstorage.h"
#include "blist.h"
#include "log.h"
#include "transport.h"
#include "spectrumbuddy.h"
#include "spectrumtimer.h"

extern LogClass Log_;

static gboolean storageTimeout(gpointer data) {
	RosterStorage *storage = (RosterStorage *) data;
	return storage->storeBuddies();
}

static void save_settings(gpointer k, gpointer v, gpointer data) {
	PurpleValue *value = (PurpleValue *) v;
	std::string key((char *) k);
	SaveData *s = (SaveData *) data;
	AbstractUser *user = s->user;
	long id = s->id;
	if (purple_value_get_type(value) == PURPLE_TYPE_BOOLEAN) {
		if (purple_value_get_boolean(value))
			Transport::instance()->sql()->addBuddySetting(user->storageId(), id, key, "1", purple_value_get_type(value));
		else
			Transport::instance()->sql()->addBuddySetting(user->storageId(), id, key, "0", purple_value_get_type(value));
	}
	else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
		const char *str = purple_value_get_string(value);
		Transport::instance()->sql()->addBuddySetting(user->storageId(), id, key, str ? str : "", purple_value_get_type(value));
	}
}

static gboolean storeAbstractSpectrumBuddy(gpointer key, gpointer v, gpointer data) {
	AbstractUser *user = (AbstractUser *) data;
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) v;
	if (s_buddy->getFlags() & SPECTRUM_BUDDY_IGNORE)
		return TRUE;
	
	// save PurpleBuddy
	std::string alias = s_buddy->getAlias();
	std::string name = s_buddy->getName();
	long id = s_buddy->getId();

	// Buddy is not in DB
	if (id != -1) {
		Transport::instance()->sql()->addBuddy(user->storageId(), name, s_buddy->getSubscription(), s_buddy->getGroup(), alias, s_buddy->getFlags());
	}
	else {
		id = Transport::instance()->sql()->addBuddy(user->storageId(), name, s_buddy->getSubscription(), s_buddy->getGroup(), alias, s_buddy->getFlags());
		s_buddy->setId(id);
	}
	Log("buddyListSaveNode", id << " " << name << " " << alias << " " << s_buddy->getSubscription());
	if (s_buddy->getBuddy() && id != -1) {
		PurpleBuddy *buddy = s_buddy->getBuddy();
		SaveData *s = new SaveData;
		s->user = user;
		s->id = id;
		g_hash_table_foreach(buddy->node.settings, save_settings, s);
		delete s;
	}
	return TRUE;
}

RosterStorage::RosterStorage(AbstractUser *user) : m_user(user) {
	m_storageCache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	m_storageTimer = new SpectrumTimer(10000, &storageTimeout, this);
}

RosterStorage::~RosterStorage() {
	delete m_storageTimer;
	g_hash_table_destroy(m_storageCache);
}

void RosterStorage::storeBuddy(AbstractSpectrumBuddy *s_buddy) {
	if (s_buddy->getFlags() & SPECTRUM_BUDDY_IGNORE)
		return;
	if (g_hash_table_lookup(m_storageCache, s_buddy->getSafeName().c_str()) == NULL)
		g_hash_table_replace(m_storageCache, g_strdup(s_buddy->getSafeName().c_str()), s_buddy);
	m_storageTimer->start();
}

void RosterStorage::storeBuddy(PurpleBuddy *buddy) {
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	storeBuddy(s_buddy);
}

bool RosterStorage::storeBuddies() {
	if (g_hash_table_size(m_storageCache) == 0) {
		return false;
	}
	
	Transport::instance()->sql()->beginTransaction();
	g_hash_table_foreach_remove(m_storageCache, storeAbstractSpectrumBuddy, m_user);
	Transport::instance()->sql()->commitTransaction();
	
	return true;
}

void RosterStorage::removeBuddy(AbstractSpectrumBuddy *s_buddy) {
	if (g_hash_table_lookup(m_storageCache, s_buddy->getSafeName().c_str()) != NULL)
		g_hash_table_remove(m_storageCache, s_buddy->getSafeName().c_str());
}

void RosterStorage::removeBuddy(PurpleBuddy *buddy) {	
	AbstractSpectrumBuddy *s_buddy = (AbstractSpectrumBuddy *) buddy->node.ui_data;
	removeBuddy(s_buddy);
}
