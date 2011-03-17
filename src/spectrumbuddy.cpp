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

#include "spectrumbuddy.h"
#include "main.h"
#include "user.h"
#include "log.h"
#include "sql.h"
#include "usermanager.h"

#include "transport.h"

SpectrumBuddy::SpectrumBuddy(long id, PurpleBuddy *buddy) : AbstractSpectrumBuddy(id), m_buddy(buddy) {
}

SpectrumBuddy::~SpectrumBuddy() {
}


std::string SpectrumBuddy::getAlias() {
	std::string alias;
	if (purple_buddy_get_alias(m_buddy))
		alias = (std::string) purple_buddy_get_alias(m_buddy);
	else
		alias = (std::string) purple_buddy_get_server_alias(m_buddy);
	return alias;
}

std::string SpectrumBuddy::getName() {
	std::string name(purple_buddy_get_name(m_buddy));
	if (name.empty()) {
		Log("SpectrumBuddy::getName", "Name is EMPTY!");
	}
	return name;
}

bool SpectrumBuddy::getStatus(PurpleStatusPrimitive &status, std::string &statusMessage) {
	PurplePresence *pres = purple_buddy_get_presence(m_buddy);
	if (pres == NULL)
		return false;
	PurpleStatus *stat = purple_presence_get_active_status(pres);
	if (stat == NULL)
		return false;
	status = purple_status_type_get_primitive(purple_status_get_type(stat));
	const char *message = purple_status_get_attr_string(stat, "message");

	if (message != NULL) {
		char *stripped = purple_markup_strip_html(message);
		statusMessage = std::string(stripped);
		g_free(stripped);
	}
	else
		statusMessage = "";
	return true;
}

bool SpectrumBuddy::getXStatus(std::string &mood, std::string &comment) {
#if defined(PURPLE_MOOD_NAME) && defined(PURPLE_MOOD_COMMENT)
	PurplePresence *pres = purple_buddy_get_presence(m_buddy);
	if (pres == NULL)
		return false;

	if (purple_presence_is_status_primitive_active(pres, PURPLE_STATUS_MOOD)) {
		PurpleStatus *stat = purple_presence_get_status(pres, "mood");
		const char *m = purple_status_get_attr_string(stat, PURPLE_MOOD_NAME);
		const char *c = purple_status_get_attr_string(stat, PURPLE_MOOD_COMMENT);
		mood = m ? m : "";
		comment = c ? c : "";
		return true;
	}
#endif
	return false;
}

std::string SpectrumBuddy::getIconHash() {
	char *avatarHash = NULL;
	PurpleBuddyIcon *icon = purple_buddy_icons_find(purple_buddy_get_account(m_buddy), purple_buddy_get_name(m_buddy));
	if (icon) {
		avatarHash = purple_buddy_icon_get_full_path(icon);
		Log(getName(), "avatarHash");
	}

	if (avatarHash) {
		Log(getName(), "Got avatar hash");
		// Check if it's patched libpurple which saves icons to directories
		char *hash = strrchr(avatarHash,'/');
		std::string h;
		if (hash) {
			char *dot;
			hash++;
			dot = strchr(hash, '.');
			if (dot)
				*dot = '\0';

			std::string ret(hash);
			g_free(avatarHash);
			return ret;
		}
		else {
			std::string ret(avatarHash);
			g_free(avatarHash);
			return ret;
		}
	}

	return "";
}

std::string SpectrumBuddy::getGroup() {
	return purple_group_get_name(purple_buddy_get_group(m_buddy)) ? std::string(purple_group_get_name(purple_buddy_get_group(m_buddy))) : std::string("Buddies");
}

std::string SpectrumBuddy::getSafeName() {
	std::string name = getName();
	Transport::instance()->protocol()->prepareUsername(name, purple_buddy_get_account(m_buddy));
	if (getFlags() & SPECTRUM_BUDDY_JID_ESCAPING) {
		name = JID::escapeNode(name);
	}
	else {
		if (name.find_last_of("@") != std::string::npos) {
			name.replace(name.find_last_of("@"), 1, "%");
		}
	}
	if (name.empty()) {
		Log("SpectrumBuddy::getSafeName", "Name is EMPTY! Previous was " << getName() << ".");
	}
	return name;
}

void SpectrumBuddy::changeGroup(std::list<std::string> &groups) {
	if (groups.size() == 0) {
		groups.push_back("Buddies");
	}

	std::list<PurpleBuddy*> to_remove;
	PurpleBuddy *alive_buddy = NULL;
	for (GSList *buddies = purple_find_buddies(purple_buddy_get_account(m_buddy), purple_buddy_get_name(m_buddy)); buddies; buddies = g_slist_delete_link(buddies, buddies)) {
		PurpleBuddy *b = (PurpleBuddy *) buddies->data;
		// check if this libpurple buddy for particular group is also in that group in roster push
		std::cout << "handling buddy in " << purple_group_get_name(purple_buddy_get_group(b)) << "\n";
		std::cout << "groups size:" << groups.size() << "\n";
		bool found = false;
		for (std::list<std::string>::iterator it = groups.begin(); it != groups.end(); ++it) {
			std::string group = *it;
			std::cout << "trying to match " << group << " and " <<  purple_group_get_name(purple_buddy_get_group(b)) << "\n";
			if (group == purple_group_get_name(purple_buddy_get_group(b))) {
				found = true;
				groups.remove(*it);
				alive_buddy = b;
				break;
			}
		}

		// this buddy has *not* been found in roster push in his group, so we have to
		// remove him from that group.
		if (!found) {
			std::cout << "not found\n";
			// we can't remove it just now, because we could loose the only one buddy.
			to_remove.push_back(b);
		}
	}

	// Remaining groups are groups where there is no buddy for this contact, so we have to add that buddy there.
	// We will use buddies from to_remove list if there are any, so we don't have to clone buddies so often.
	for (std::list<std::string>::iterator it = groups.begin(); it != groups.end(); ++it) {
		std::string group = *it;
		PurpleGroup *g = purple_find_group(group.c_str());
		if (!g) {
			g = purple_group_new(group.c_str());
			purple_blist_add_group(g, NULL);
		}
		// there's no buddy marked to remove, so we have to create new one and add it to group
		if (to_remove.empty()) {
			PurpleContact *contact = purple_contact_new();
			purple_blist_add_contact(contact, g, NULL);

			// create buddy
			PurpleBuddy *buddy = purple_buddy_new(purple_buddy_get_account(m_buddy), getName().c_str(), getAlias().c_str());
			purple_blist_add_buddy(buddy, contact, g, NULL);
			purple_account_add_buddy(purple_buddy_get_account(m_buddy), buddy);
			alive_buddy = buddy;
		}
		else {
			PurpleBuddy *b = to_remove.back();
			to_remove.pop_back();
			purple_blist_add_contact(purple_buddy_get_contact(b), g, NULL);
			alive_buddy = b;
		}
	}

	// remove buddies marked for removing
	for (std::list<PurpleBuddy*>::const_iterator it = to_remove.begin(); it != to_remove.end(); ++it) {
		// change our buddy if this one is marked for deletion
		if (*it == m_buddy) {
			// switch ui_data
			void *data = m_buddy->node.ui_data;
			m_buddy->node.ui_data = alive_buddy->node.ui_data;
			alive_buddy->node.ui_data = data;
			m_buddy = alive_buddy;
		}
		purple_account_remove_buddy(purple_buddy_get_account(m_buddy), *it, purple_buddy_get_group(*it));
		purple_blist_remove_buddy(*it);
	}
}

void SpectrumBuddy::changeAlias(const std::string &alias) {
	if (alias != getAlias()) {
		purple_blist_alias_buddy(m_buddy, alias.c_str());
		purple_blist_server_alias_buddy(m_buddy, alias.c_str());
		serv_alias_buddy(m_buddy);
	}
}

void SpectrumBuddy::handleBuddyRemoved(PurpleBuddy *buddy) {
	// it's not our buddy who is being removed, so it's ok
	if (buddy != m_buddy)
		return;
	// find alive buddy
	PurpleBuddy *alive_buddy = NULL;
	for (GSList *buddies = purple_find_buddies(purple_buddy_get_account(m_buddy), purple_buddy_get_name(m_buddy)); buddies; buddies = g_slist_delete_link(buddies, buddies)) {
		PurpleBuddy *b = (PurpleBuddy *) buddies->data;
		if (b != buddy) {
			alive_buddy = b;
			break;
		}
	}
	// switch them
	if (alive_buddy) {
		void *data = m_buddy->node.ui_data;
		m_buddy->node.ui_data = alive_buddy->node.ui_data;
		alive_buddy->node.ui_data = data;
		m_buddy = alive_buddy;
	}
}
