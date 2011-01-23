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
#include "spectrum_util.h"

SettingsManager::SettingsManager(AbstractUser *user) {
	m_user = user;
	m_settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
}

SettingsManager::~SettingsManager() {
	g_hash_table_destroy(m_settings);
}

template <>
void SettingsManager::addSetting(const std::string &key, const bool &value) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		Transport::instance()->sql()->addSetting(m_user->storageId(), key, value ? "1" : "0", PURPLE_TYPE_BOOLEAN);
		v = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(v, value);
		g_hash_table_replace(m_settings, g_strdup(key.c_str()), v);
	}
}

template <>
void SettingsManager::addSetting(const std::string &key, const std::string &value) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		Transport::instance()->sql()->addSetting(m_user->storageId(), key, value, PURPLE_TYPE_STRING);
		v = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(v, value.c_str());
		g_hash_table_replace(m_settings, g_strdup(key.c_str()), v);
	}
}

template <>
void SettingsManager::addSetting(const std::string &key, const int &value) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		Transport::instance()->sql()->addSetting(m_user->storageId(), key, stringOf(value), PURPLE_TYPE_INT);
		v = purple_value_new(PURPLE_TYPE_INT);
		purple_value_set_int(v, value);
		g_hash_table_replace(m_settings, g_strdup(key.c_str()), v);
	}
}

template<>
bool SettingsManager::getSetting(const std::string &key, const bool &def) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		return def;
	}
	return purple_value_get_boolean(v);
}

template<>
std::string SettingsManager::getSetting(const std::string &key, const std::string &def) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		return def;
	}
	return purple_value_get_string(v);
}

template<>
int SettingsManager::getSetting(const std::string &key, const int &def) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		return def;
	}
	return purple_value_get_int(v);
}

template <>
void SettingsManager::updateSetting(const std::string &key, const bool &value) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		return;
	}
	purple_value_set_boolean(v, value);
	Transport::instance()->sql()->updateSetting(m_user->storageId(), key, value ? "1" : "0");
}

template <>
void SettingsManager::updateSetting(const std::string &key, const std::string &value) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		return;
	}
	purple_value_set_string(v, value.c_str());
	Transport::instance()->sql()->updateSetting(m_user->storageId(), key, value);
}

template <>
void SettingsManager::updateSetting(const std::string &key, const int &value) {
	PurpleValue *v;
	if ((v = (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str())) == NULL) {
		return;
	}
	purple_value_set_int(v, value);
	Transport::instance()->sql()->updateSetting(m_user->storageId(), key, stringOf(value));
}

void SettingsManager::setSettings(GHashTable *settings) {
	if (m_settings)
		g_hash_table_destroy(m_settings);
	m_settings = settings;
}

PurpleValue *SettingsManager::getSettingValue(const std::string &key) {
	return (PurpleValue *) g_hash_table_lookup(m_settings, key.c_str());
}
