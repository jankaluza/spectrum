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

#ifndef _HI_ADHOC_REPEATER_H
#define _HI_ADHOC_REPEATER_H

#include <string>
#include "account.h"
#include "user.h"
#include "glib.h"
#include "request.h"
#include "adhoccommandhandler.h"

class GlooxMessageHandler;
class User;

/*
 * Handler for PURPLE_REQUESTs. It handles different requests and resends them as Ad-Hoc commands.
 */
class AdhocRepeater : public AdhocCommandHandler
{
	public:
		// PURPLE_REQUEST_INPUT
		AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, const std::string &value, gboolean multiline, gboolean masked, GCallback ok_cb, GCallback cancel_cb, void * user_data);
		// PURPLE_REQUEST_ACTION
		AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, int default_action, void * user_data, size_t action_count, va_list actions);
		// PURPLE_REQUEST_FIELDS
		AdhocRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, PurpleRequestFields *fields, GCallback ok_cb, GCallback cancel_cb, void * user_data);
		~AdhocRepeater();
		bool handleIq(const IQ &iq);

		void setType(PurpleRequestType t) { m_type = t; }
		PurpleRequestType type() { return m_type; }
		const std::string & from() { return m_from; }

	private:
		GlooxMessageHandler *main;				// client
		User *m_user;							// User to which Ad-Hoc are sent
		void *m_requestData;					// user_data from PURPLE_REQUEST
		GCallback m_ok_cb;						// callback which is called when user accepts the dialog
		GCallback m_cancel_cb;					// callback which is called when user rejects the dialog
		PurpleRequestType m_type;				// type of request
		std::string m_from;						// full jid from which initial IQ-get was sent
		std::map<int, GCallback> m_actions;		// callbacks for PURPLE_REQUEST_ACTION
		PurpleRequestFields *m_fields;			// fields for PURPLE_REQUEST_FIELDS
		std::string m_defaultString;			// default value of PURPLE_REQUEST_INPUT
};

#endif
