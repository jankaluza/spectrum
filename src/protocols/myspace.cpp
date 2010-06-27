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

#include "myspace.h"
#include "../main.h"

MyspaceProtocol::MyspaceProtocol(GlooxMessageHandler *main){
	m_main = main;
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("jabber:iq:gateway");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");
	m_transportFeatures.push_back("jabber:iq:search");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

MyspaceProtocol::~MyspaceProtocol() {}

bool MyspaceProtocol::isValidUsername(const std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> MyspaceProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> MyspaceProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string MyspaceProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your Myspace name and password:");
	else if (key == "username")
		return _("Myspaec name");
	return "not defined";
}

void MyspaceProtocol::onConnected(AbstractUser *user) {
	if (purple_account_get_connection(user->account())) {
		PurpleConnection *gc = purple_account_get_connection(user->account());
		PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
		if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
			PurplePluginAction *action = NULL;
			GList *actions, *l;

			actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

			for (l = actions; l != NULL; l = l->next) {
				if (l->data) {
					action = (PurplePluginAction *) l->data;
					action->plugin = plugin;
					action->context = gc;
					if ((std::string) action->label == "Add friends from MySpace.com") {
						action->callback(action);
					}
					purple_plugin_action_free(action);
				}
			}
		}
	}
}
