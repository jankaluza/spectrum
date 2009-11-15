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

#ifndef SPECTRUM_BUDDY_H
#define SPECTRUM_BUDDY_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>

class User;

using namespace gloox;

typedef enum { 	SUBSCRIPTION_NONE = 0,
				SUBSCRIPTION_TO,
				SUBSCRIPTION_FROM,
				SUBSCRIPTION_BOTH,
				SUBSCRIPTION_ASK
				} SpectrumSubscriptionType;

// Function used as GDestroyNotify for GHashTable
void SpectrumBuddyFree(gpointer data);

// Wrapper for PurpleBuddy
class SpectrumBuddy {
	public:
		SpectrumBuddy();
		SpectrumBuddy(const std::string &uin);
		SpectrumBuddy(User *user, PurpleBuddy *buddy);
		~SpectrumBuddy();

		void setUser(User *user);
		User *getUser();

		void setBuddy(PurpleBuddy *buddy);
		
		void setOffline();
		void setOnline();
		bool isOnline();

		void setId(long id);
		long getId();

		void setUin(const std::string &uin);
		const std::string &getUin();
		const std::string getSafeUin();

		void setSubscription(SpectrumSubscriptionType subscription);
		SpectrumSubscriptionType &getSubscription();

		void setNickname(const std::string &nickname);
		const std::string &getNickname();

		void setGroup(const std::string &group);
		const std::string &getGroup();

		const std::string getJid();
		const std::string getBareJid();
		bool getStatus(int &status, std::string &message);

		void store(PurpleBuddy *buddy = NULL);
		void remove();

	private:
		long m_id;
		std::string m_uin;
		std::string m_nickname;
		std::string m_group;
		SpectrumSubscriptionType m_subscription;
		bool m_online;
		std::string m_lastPresence;
		PurpleBuddy *m_buddy;
		User *m_user;
};

#endif
