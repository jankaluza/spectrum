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
#include "adhocsettings.h"
#include "gloox/disconodehandler.h"
#include "gloox/adhoc.h"
 
static AdhocCommandHandler * createSettingsHandler(GlooxMessageHandler *m, User *user, const std::string &from, const std::string &id) {
	AdhocCommandHandler *handler = new AdhocSettings(m, user, from, id);
	return handler;
}
 
GlooxAdhocHandler::GlooxAdhocHandler(GlooxMessageHandler *m) {
	main = m;
	main->j->registerIqHandler( this, ExtAdhocCommand );
	main->j->registerStanzaExtension( new Adhoc::Command() );
	main->j->disco()->addFeature( XMLNS_ADHOC_COMMANDS );
	main->j->disco()->registerNodeHandler( this, XMLNS_ADHOC_COMMANDS );
	main->j->disco()->registerNodeHandler( this, std::string() );
	
	m_handlers["transport_settings"].name = "Transport settings";
	m_handlers["transport_settings"].createHandler = createSettingsHandler;
}

GlooxAdhocHandler::~GlooxAdhocHandler() { }

StringList GlooxAdhocHandler::handleDiscoNodeFeatures (const JID &from, const std::string &node) {
	StringList features;
	features.push_back( XMLNS_ADHOC_COMMANDS );
	return features;
}

/*
 * Returns ItemList which contains list of available commands
 */
Disco::ItemList GlooxAdhocHandler::handleDiscoNodeItems( const JID &_from, const JID &_to, const std::string& node ) {
	Disco::ItemList lst;
	std::string from = _from.bare();
	std::string to = _to.bare();
	Log().Get("GlooxAdhocHandler") << "items" << from << to;

	// it's adhoc request for transport
	if (to == main->jid()) {
		User *user = main->userManager()->getUserByJID(from);
		if (user) {
			// add internal commands from m_handlers
			for(std::map<std::string, adhocCommand>::iterator u = m_handlers.begin(); u != m_handlers.end() ; u++) {
				lst.push_back( new Disco::Item( main->jid(), (*u).first, (std::string) tr(user->getLang(), (*u).second.name) ) );
			}
			// add commands from libpurple
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
							// we are using "transport_" prefix here to identify command in disco#info handler
							// it's ugly and we're bastards, but it's the easy solution
							lst.push_back( new Disco::Item( main->jid(), "transport_" + (std::string) action->label,(std::string) tr(user->getLang(), action->label) ) );
							purple_plugin_action_free(action);
						}
					}
				}
			}
		}
	}
	// it's request for some PurpleBuddy
	else {
		User *user = main->userManager()->getUserByJID(from);
		if (user) {
			if (user->isConnected() && purple_account_get_connection(user->account())) {
				Log().Get("GlooxAdhocHandler") << user->getLang();
				GList *l, *ll;
				PurpleConnection *gc = purple_account_get_connection(user->account());
				PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
				PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);

				if(!prpl_info || !prpl_info->blist_node_menu)
					return lst;
				std::string name(JID(to).username());
				std::for_each( name.begin(), name.end(), replaceJidCharacters() );
				PurpleBuddy *buddy = purple_find_buddy(user->account(), name.c_str());

				for(l = ll = prpl_info->blist_node_menu((PurpleBlistNode*)buddy); l; l = l->next) {
					PurpleMenuAction *action = (PurpleMenuAction *) l->data;
					lst.push_back( new Disco::Item( _to.bare(), "transport_" + (std::string) action->label, (std::string) tr(user->getLang(), action->label) ) );
					purple_menu_action_free(action);
				}
			}
		}
	}
	return lst;
}

/*
 * Returns IdentityList (copied from Gloox :) )
 */
Disco::IdentityList GlooxAdhocHandler::handleDiscoNodeIdentities( const JID& jid, const std::string& node ) {
	Disco::IdentityList l;
	l.push_back( new Disco::Identity( "automation",node == XMLNS_ADHOC_COMMANDS ? "command-list" : "command-node",node == XMLNS_ADHOC_COMMANDS ? "Ad-Hoc Commands" : "") );
	return l;
}

bool GlooxAdhocHandler::handleIq( const IQ &stanza ) {
	Log().Get("GlooxAdhocHandler") << "handleIq";
	std::string to = stanza.to().bare();
	std::string from = stanza.from().bare();
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag) return false;
	Tag *tag = stanzaTag->findChild( "command" );
	if (!tag) { Log().Get("GlooxAdhocHandler") << "No Node!"; return false; }
	const std::string& node = tag->findAttribute( "node" );
	if (node.empty()) {
		delete stanzaTag;
		return false;
	}

	User *user = main->userManager()->getUserByJID(from);
	if (user) {
		// check if we have existing session for this jid
		if (hasSession(stanza.from().full())) {
			if( m_sessions[stanza.from().full()]->handleIq(stanza) ) {
				// AdhocCommandHandler returned true, so we should remove it
				std::string jid = stanza.from().full();
				delete m_sessions[jid];
				unregisterSession(jid);
			}
			delete stanzaTag;
			return true;
		}
		// check if we have registered handler for this node and create AdhocCommandHandler class
		else if (m_handlers.find(node) != m_handlers.end()) {
			AdhocCommandHandler *handler = m_handlers[node].createHandler(main, user, stanza.from().full(), stanza.id());
			registerSession(stanza.from().full(), handler);
			delete stanzaTag;
			return true;
		}
		// if it's PurpleAction, so we don't have handler for it, we have to execute the action here.
		if (user->isConnected() && purple_account_get_connection(user->account())) {
			if (to == main->jid()) {
				PurpleConnection *gc = purple_account_get_connection(user->account());
				PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
				if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
					PurplePluginAction *action = NULL;
					GList *actions, *l;

					actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

					for (l = actions; l != NULL; l = l->next) {
						if (l->data) {
							action = (PurplePluginAction *) l->data;
							if (node == "transport_" + (std::string) action->label) {
								AdhocData data;
								data.id = stanza.id();
								data.from = stanza.from().full();
								data.node = node;
								data.callerType = CALLER_ADHOC;
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
				std::string name(JID(to).username());
				std::for_each( name.begin(), name.end(), replaceJidCharacters() );
				PurpleBuddy *buddy = purple_find_buddy(user->account(), name.c_str());

				for(l = ll = prpl_info->blist_node_menu((PurpleBlistNode*)buddy); l; l = l->next) {
					PurpleMenuAction *action = (PurpleMenuAction *) l->data;
					if (node == "transport_" + (std::string) action->label) {
						void (*callback)(PurpleBlistNode *, gpointer);
						callback = (void (*)(PurpleBlistNode *, gpointer))action->callback;
						if (callback)
							callback((PurpleBlistNode*)buddy, action->data);

						IQ _s(IQ::Result, stanza.from().full(), stanza.id());
						_s.setFrom(stanza.to());
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
	delete stanzaTag;
	return true;
}

bool GlooxAdhocHandler::hasSession(const std::string &jid) {
	std::map<std::string,AdhocCommandHandler *> ::iterator iter = m_sessions.begin();
	iter = m_sessions.find(jid);
	if(iter != m_sessions.end())
		return true;
	return false;
}

void GlooxAdhocHandler::handleIqID( const IQ &iq, int context ) {}

void GlooxAdhocHandler::handleDiscoInfo(const JID &jid, const Disco::Info &info, int context) {
	Log().Get("handle disco info adhoc") << jid.full();
}

void GlooxAdhocHandler::handleDiscoItems(const JID &jid, const Disco::Items &items, int context) {}
void GlooxAdhocHandler::handleDiscoError(const JID &jid, const Error *error, int context) {}


