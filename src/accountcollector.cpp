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

#include "accountcollector.h"
#include "request.h"

static gboolean collect_account(gpointer key, gpointer v, gpointer data) {
	AccountCollector *collector = (AccountCollector*) data;
	PurpleAccount *account = (PurpleAccount *) v;
	collector->collectNow(account, false);
	return TRUE;
}

static gboolean collectorTimeout(gpointer data){
	AccountCollector *collector = (AccountCollector*) data;
	collector->timeout();
	return TRUE;
}

AccountCollector::AccountCollector() {
	m_accounts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	//purple_timeout_add_seconds(5*60, &collectorTimeout, this);
	
}

AccountCollector::~AccountCollector() {
	g_hash_table_destroy(m_accounts);
}

void AccountCollector::stopCollecting(PurpleAccount *account) {
	return;
	if (g_hash_table_lookup(m_accounts, purple_account_get_username(account)) != NULL)
		g_hash_table_remove(m_accounts, purple_account_get_username(account));
}


void AccountCollector::collect(PurpleAccount *account) {
	return;
	if (g_hash_table_lookup(m_accounts, purple_account_get_username(account)) == NULL)
		g_hash_table_replace(m_accounts, g_strdup(purple_account_get_username(account)), account);
}

void AccountCollector::collectNow(PurpleAccount *account, bool remove) {
	return;
	if (account->ui_data == NULL) {
		Log("AccountCollector","freeing account " << purple_account_get_username(account));
		
		if (remove)
			g_hash_table_remove(m_accounts, purple_account_get_username(account));

		purple_account_set_enabled(account, purple_core_get_ui(), FALSE);

		purple_notify_close_with_handle(account);
		purple_request_close_with_handle(account);

		purple_accounts_remove(account);

		GSList *buddies = purple_find_buddies(account, NULL);
		while(buddies) {
			PurpleBuddy *b = (PurpleBuddy *) buddies->data;
			purple_blist_remove_buddy(b);
			buddies = g_slist_delete_link(buddies, buddies);
		}

		/* Remove any open conversation for this account */
		for (GList *it = purple_get_conversations(); it; ) {
			PurpleConversation *conv = (PurpleConversation *) it->data;
			it = it->next;
			if (purple_conversation_get_account(conv) == account)
				purple_conversation_destroy(conv);
		}

		/* Remove this account's pounces */
 			// purple_pounce_destroy_all_by_account(account);

		/* This will cause the deletion of an old buddy icon. */
		purple_buddy_icons_set_account_icon(account, NULL, 0);

		purple_account_destroy(account);
	}
}

void AccountCollector::timeout() {
	g_hash_table_foreach_remove(m_accounts, collect_account, this);
}

