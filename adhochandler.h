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
 
#ifndef _HI_ADHOC_HANDLER_H
#define _HI_ADHOC_HANDLER_H

#include <string>
#include "account.h"
#include "user.h"
#include "glib.h"

class GlooxMessageHandler;

class AdhocHandler : public DiscoNodeHandler, public DiscoHandler, public IqHandler
{
	public:
		AdhocHandler(GlooxMessageHandler *m);
		~AdhocHandler();
		virtual StringList handleDiscoNodeFeatures( const std::string& node );
		virtual StringMap handleDiscoNodeIdentities( const std::string& node, std::string& name );
		virtual DiscoNodeItemList handleDiscoNodeItems( const std::string &from, const std::string &to, const std::string& node );
		virtual bool handleIq( Stanza *stanza );
		virtual bool handleIqID( Stanza *stanza, int context );
		virtual void handleDiscoInfoResult( Stanza *stanza, int context );
		virtual void handleDiscoItemsResult( Stanza *stanza, int context );
		virtual void handleDiscoError( Stanza *stanza, int context );

	private:
		GlooxMessageHandler *main;
	
};

#endif
 