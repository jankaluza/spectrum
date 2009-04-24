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

#include "autoconnectloop.h"
#include "main.h"
#include "log.h"
#include "usermanager.h"

/*
 * Callback which is called periodically and restoring connections.
 */
static gboolean iter(gpointer data){
	AutoConnectLoop *loop = (AutoConnectLoop*) data;
	if (loop->restoreNextConnection())
		return TRUE;
	delete loop;
	return FALSE;
}

AutoConnectLoop::AutoConnectLoop(GlooxMessageHandler *m){
	main = m;
	g_timeout_add(10000,&iter,this);
}

AutoConnectLoop::~AutoConnectLoop() {
	Log().Get("connection restorer") << "Restorer deleted";
}

bool AutoConnectLoop::restoreNextConnection() {
	Stanza * stanza;
	PurpleAccount *account;
	GList *l;
	User *user;
	Log().Get("connection restorer") << "Restoring new connection";
	l = purple_accounts_get_all();
	if (l==NULL)
		return true;
	for (l; l != NULL; l = l->next)
	{
		account = (PurpleAccount *)l->data;
		std::map<std::string,int>::iterator it = m_probes.find(JID((std::string)purple_account_get_string(account,"lastUsedJid","")).bare()); //See Find in the Algorithms section.
		if( it == m_probes.end() )
		{
			Log().Get("connection restorer") << "Checking next account";
			m_probes[JID((std::string)purple_account_get_string(account,"lastUsedJid","")).bare()]=1;
			if (purple_presence_is_online(account->presence))
			{
				user = main->userManager()->getUserByAccount(account);
				if (user==NULL){
					Log().Get("connection restorer") << "Sending probe presence to "<< JID((std::string)purple_account_get_string(account,"lastUsedJid","")).bare();
					stanza = new Stanza("presence");
					stanza->addAttribute( "to", JID((std::string)purple_account_get_string(account,"lastUsedJid","")).bare());
					stanza->addAttribute( "type", "probe");
					stanza->addAttribute( "from", main->jid());
					main->j->send(stanza);
					return true;
				}
			}
		}
	}
	Log().Get("connection restorer") << "There is no other account to be checked => stopping Restorer";
	return false;
}
