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

#include "settingsmanager.h"
#include "log.h"
#include "transport.h"

SettingsManager::SettingsManager(AbstractUser *user) {
	m_user = user;
}

SettingsManager::~SettingsManager() {
	
}
/*
void SettingsManager::addSetting(const std::string &key, bool value) {
	PurpleValue *value;
	if ((value = getSetting(key)) == NULL) {
		Transport::instance()->sql()->addSetting(m_userID, key, value ? "1" : "0", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, value);
		g_hash_table_replace(m_settings, g_strdup(key.c_str()), value);
	}
}*/
