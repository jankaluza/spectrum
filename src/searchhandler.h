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

#ifndef _HI_SEARCH_HANDLER_H
#define _HI_SEARCH_HANDLER_H

#include "purple.h"
#include <gloox/iqhandler.h>
#include "notify.h"

class GlooxMessageHandler;
class SearchRepeater;
using namespace gloox;


class SearchExtension : public StanzaExtension
{

public:
	/**
	* Constructs a new object from the given Tag.
	*/
	SearchExtension();
	SearchExtension(const Tag *tag);

	/**
	* Virtual Destructor.
	*/
	virtual ~SearchExtension();

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
	return new SearchExtension(tag);
	}

	// reimplemented from StanzaExtension
	virtual Tag* tag() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		return new SearchExtension(m_tag);
	}

private:
	Tag *m_tag;

};

class GlooxSearchHandler : public IqHandler
{

public:
	GlooxSearchHandler(GlooxMessageHandler *parent);
	~GlooxSearchHandler();
	bool handleIq (const IQ &iq);
	void handleIqID (const IQ &iq, int context);
	void searchResultsArrived(const std::string &from, PurpleNotifySearchResults *results);

	void registerSession(const std::string &jid, SearchRepeater *handler) {m_sessions[jid] = handler; }
	void unregisterSession(std::string &jid) { m_sessions.erase(jid); }
	bool hasSession(const std::string &jid);

private:
	std::map<std::string, SearchRepeater *> m_sessions;
	GlooxMessageHandler *p;
};

#endif