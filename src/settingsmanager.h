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

#ifndef SPECTRUM_SETTINGSMANAGER_H
#define SPECTRUM_SETTINGSMANAGER_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "abstractuser.h"

using namespace gloox;


// Stores and manages users' settings .
class SettingsManager {
	public:
		SettingsManager(AbstractUser *user);
		virtual ~SettingsManager();

		// Adds new boolean setting. It does nothing if setting already exists.
		template <typename T> void addSetting(const std::string &key, const T &value);

		// Returns setting. If if doesn't exist, return default value defined by 'def'.
		template<typename T> T getSetting(const std::string& key, const T& def = T());

		template <typename T> void updateSetting(const std::string &key, const T &value);

		PurpleValue *getSettingValue(const std::string &key);

		// Replaces all settings and sets the new ones.
		void setSettings(GHashTable *settings);

	private:
		AbstractUser *m_user;
		GHashTable *m_settings;
};

#endif
