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
 
AdhocHandler::AdhocHandler(GlooxMessageHandler *m) {
	main = m;
	main->j->registerIqHandler( this, XMLNS_ADHOC_COMMANDS );
	main->j->disco()->addFeature( XMLNS_ADHOC_COMMANDS );
	main->j->disco()->registerNodeHandler( this, XMLNS_ADHOC_COMMANDS );
	main->j->disco()->registerNodeHandler( this, std::string() );

}

AdhocHandler::~AdhocHandler() { }

StringList AdhocHandler::handleDiscoNodeFeatures( const std::string& /*node*/ ) {
	StringList features;
	features.push_back( XMLNS_ADHOC_COMMANDS );
	return features;
}

DiscoNodeItemList AdhocHandler::handleDiscoNodeItems( const std::string &from, const std::string &to, const std::string& node ) {
	DiscoNodeItemList lst;
	if (node.empty()) {
		DiscoNodeItem item;
		item.node = XMLNS_ADHOC_COMMANDS;
		item.jid = main->jid();
		item.name = "Ad-Hoc Commands";
		lst.push_back( item );
	}
	else if (node == XMLNS_ADHOC_COMMANDS) {
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
								DiscoNodeItem item;
								item.node = (std::string) action->label;
								item.jid = main->jid();
								item.name = (std::string) action->label;
								purple_plugin_action_free(action);
								lst.push_back( item );
							}
						}
					}
				}
			}
		}
	}
	return lst;
}

StringMap AdhocHandler::handleDiscoNodeIdentities( const std::string& node, std::string& name ) {
	name = (std::string) node;

	StringMap ident;
	if( node == XMLNS_ADHOC_COMMANDS )
		ident["automation"] = "command-list";
	else
		ident["automation"] = "command-node";
	return ident;
}

bool AdhocHandler::handleIq( Stanza *stanza ) {
	return true;
}

bool AdhocHandler::handleIqID( Stanza *stanza, int context ) {
	return true;
}

void AdhocHandler::handleDiscoInfoResult( Stanza *stanza, int context ) {
	
}

void AdhocHandler::handleDiscoItemsResult( Stanza *stanza, int context ) {
	
}

void AdhocHandler::handleDiscoError( Stanza *stanza, int context ) {
	
}


