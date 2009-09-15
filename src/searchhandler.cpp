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

#include "searchhandler.h"
#include "main.h"
#include "gloox/search.h"
#include "usermanager.h"
#include "protocols/abstractprotocol.h"
#include "log.h"
#include "searchrepeater.h"

SearchExtension::SearchExtension() : StanzaExtension( ExtSearch )
{
	m_tag = NULL;
}

SearchExtension::SearchExtension(const Tag *tag) : StanzaExtension( ExtSearch )
{
	m_tag = tag->clone();
}

SearchExtension::~SearchExtension()
{
	Log().Get("SearchExtension") << "deleting SearchExtension()";
	delete m_tag;
}

const std::string& SearchExtension::filterString() const
{
	static const std::string filter = "iq/query[@xmlns='jabber:iq:search']";
	return filter;
}

Tag* SearchExtension::tag() const
{
	return m_tag->clone();
}

GlooxSearchHandler::GlooxSearchHandler(GlooxMessageHandler *parent) : IqHandler() {
	p=parent;
	p->j->registerStanzaExtension( new SearchExtension() );
}

GlooxSearchHandler::~GlooxSearchHandler() {
}

bool GlooxSearchHandler::hasSession(const std::string &jid) {
	std::map<std::string,SearchRepeater *> ::iterator iter = m_sessions.begin();
	iter = m_sessions.find(jid);
	return iter != m_sessions.end();
}

void GlooxSearchHandler::searchResultsArrived(const std::string &from, PurpleNotifySearchResults *results) {
	if (hasSession(from)) {
		m_sessions[from]->sendSearchResults(results);
	}
}

bool GlooxSearchHandler::handleIq (const IQ &iq) {
	User *user = p->userManager()->getUserByJID(iq.from().bare());
	if (!user) return true;

	if(iq.subtype() == IQ::Get) {
		AdhocData data;
		data.id = iq.id();
		data.from = iq.from().full();
		data.node = p->protocol()->userSearchAction();
		data.callerType = CALLER_SEARCH;
		user->setAdhocData(data);
		PurpleConnection *gc = purple_account_get_connection(user->account());
		PurplePlugin *plugin = gc && PURPLE_CONNECTION_IS_CONNECTED(gc) ? gc->prpl : NULL;
		if (plugin && PURPLE_PLUGIN_HAS_ACTIONS(plugin)) {
			PurplePluginAction *action = NULL;
			GList *actions, *l;

			actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

			for (l = actions; l != NULL; l = l->next) {
				if (l->data) {
					action = (PurplePluginAction *) l->data;
					if (data.node == (std::string) action->label) {
						action->plugin = plugin;
						action->context = gc;
						action->callback(action);
						purple_plugin_action_free(action);
						break;
					}
					purple_plugin_action_free(action);
				}
			}
		}
	}
	else {
		if (hasSession(iq.from().full())) {
			m_sessions[iq.from().full()]->handleIq(iq);
		}
	}
	return true;
}

void GlooxSearchHandler::handleIqID (const IQ &iq, int context) { }
