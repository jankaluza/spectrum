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
 
 #include "adhochandler.h"
 #include "usermanager.h"
 #include "log.h"
 #include "adhocrepeater.h"
 #include "gloox/disconodehandler.h"
 
AdhocHandler::AdhocHandler(GlooxMessageHandler *m) {
	main = m;
	main->j->registerIqHandler( this, ExtAdhocCommand );
	main->j->disco()->addFeature( XMLNS_ADHOC_COMMANDS );
	main->j->disco()->registerNodeHandler( this, XMLNS_ADHOC_COMMANDS );
	main->j->disco()->registerNodeHandler( this, std::string() );

}

AdhocHandler::~AdhocHandler() { }

StringList AdhocHandler::handleDiscoNodeFeatures (const JID &from, const std::string &node) {
	StringList features;
	features.push_back( XMLNS_ADHOC_COMMANDS );
	return features;
}

Disco::ItemList AdhocHandler::handleDiscoNodeItems( const JID &_from, const JID &_to, const std::string& node ) {
	Disco::ItemList lst;
	std::string from = _from.bare();
	std::string to = _to.bare();
// 	if (node.empty()) {
// 		lst.push_back( new Disco::Item( main->jid(), XMLNS_ADHOC_COMMANDS, "Ad-Hoc Commands" ) );
// 	}
// 	else if (node == XMLNS_ADHOC_COMMANDS) {
		if (to == main->jid()) {
			User *user = main->userManager()->getUserByJID(from);
			if (user) {
				if (user->isConnected() && purple_account_get_connection(user->account())) {
					PurpleConnection *gc = purple_account_get_connection(user->account());
					PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
					if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
						PurplePluginAction *action = NULL;
						GList *actions, *l;

						actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

						for (l = actions; l != NULL; l = l->next) {
							if (l->data) {
								action = (PurplePluginAction *) l->data;
								lst.push_back( new Disco::Item( main->jid(), (std::string) action->label, (std::string) action->label ) );
								purple_plugin_action_free(action);
							}
						}
					}
				}
			}
		}
		else {
			User *user = main->userManager()->getUserByJID(from);
			if (user) {
				if (user->isConnected() && purple_account_get_connection(user->account())) {
					GList *l, *ll;
					PurpleConnection *gc = purple_account_get_connection(user->account());
					PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
					PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);

					if(!prpl_info || !prpl_info->blist_node_menu)
						return lst;

					PurpleBuddy *buddy = purple_find_buddy(user->account(), JID(to).username().c_str());

					for(l = ll = prpl_info->blist_node_menu((PurpleBlistNode*)buddy); l; l = l->next) {
						PurpleMenuAction *action = (PurpleMenuAction *) l->data;
						lst.push_back( new Disco::Item( main->jid(), (std::string) action->label, (std::string) action->label ) );
						purple_menu_action_free(action);
					}
				}
			}
		}
// 	}
	return lst;
}

Disco::IdentityList AdhocHandler::handleDiscoNodeIdentities( const JID& jid, const std::string& node ) {
	Disco::IdentityList l;
	l.push_back( new Disco::Identity( "automation",node == XMLNS_ADHOC_COMMANDS ? "command-list" : "command-node",node == XMLNS_ADHOC_COMMANDS ? "Ad-Hoc Commands" : "") );
	return l;
}

bool AdhocHandler::handleIq( const IQ &stanza ) {
	Log().Get("AdhocHandler") << "handleIq";
	std::string to = stanza.to().bare();
	std::string from = stanza.from().bare();
	Tag *tag = stanza.tag()->findChild( "command" );
	const std::string& node = tag->findAttribute( "node" );
	if (node.empty()) return false;

	User *user = main->userManager()->getUserByJID(from);
	if (user) {
		if (user->isConnected() && purple_account_get_connection(user->account())) {
			if (hasSession(stanza.from().full())) {
				return m_sessions[stanza.from().full()]->handleIq(stanza);
			}
			else if (to == main->jid()) {
				PurpleConnection *gc = purple_account_get_connection(user->account());
				PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
				if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
					PurplePluginAction *action = NULL;
					GList *actions, *l;

					actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

					for (l = actions; l != NULL; l = l->next) {
						if (l->data) {
							action = (PurplePluginAction *) l->data;
							if (node == (std::string) action->label) {
								AdhocData data;
								data.id = stanza.id();
								data.from = stanza.from().full();
								data.node = node;
								user->setAdhocData(data);
								action->plugin = plugin;
								action->context = gc;
								action->callback(action);
							}
							purple_plugin_action_free(action);
						}
					}
				}
			}
			else {
				GList *l, *ll;
				PurpleConnection *gc = purple_account_get_connection(user->account());
				PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
				PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);

				if(!prpl_info || !prpl_info->blist_node_menu)
					return true;
				PurpleBuddy *buddy = purple_find_buddy(user->account(), JID(to).username().c_str());

				for(l = ll = prpl_info->blist_node_menu((PurpleBlistNode*)buddy); l; l = l->next) {
					PurpleMenuAction *action = (PurpleMenuAction *) l->data;
					if (node == (std::string) action->label) {
						void (*callback)(PurpleBlistNode *, gpointer);
						callback = (void (*)(PurpleBlistNode *, gpointer))action->callback;
						if (callback)
							callback((PurpleBlistNode*)buddy, action->data);

						IQ _s(IQ::Result, stanza.from().full(), stanza.id());
						_s.setFrom(main->jid());
						Tag *s = _s.tag();

						Tag *c = new Tag("command");
						c->addAttribute("xmlns","http://jabber.org/protocol/commands");
						c->addAttribute("sessionid",tag->findAttribute("sessionid"));
						c->addAttribute("node","configuration");
						c->addAttribute("status","completed");
						main->j->send(s);

					}
					purple_menu_action_free(action);
				}
			}
		}
	}
	return true;
}

bool AdhocHandler::hasSession(const std::string &jid) {
	std::map<std::string,AdhocRepeater *> ::iterator iter = m_sessions.begin();
	iter = m_sessions.find(jid);
	if(iter != m_sessions.end())
		return true;
	return false;
}

void AdhocHandler::handleIqID( const IQ &iq, int context ) {
}

void AdhocHandler::handleDiscoInfo(const JID &jid, const Disco::Info &info, int context) {}
void AdhocHandler::handleDiscoItems(const JID &jid, const Disco::Items &items, int context) {}
void AdhocHandler::handleDiscoError(const JID &jid, const Error *error, int context) {}


