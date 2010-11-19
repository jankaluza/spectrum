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

#ifndef _HI_CL_H
#define _HI_CL_H

#include "Swiften/Swiften.h"
#include "Swiften/Queries/GetResponder.h"
#include "Swiften/Queries/SetResponder.h"
#include "Swiften/Elements/InBandRegistrationPayload.h"

struct UserRow;

class SpectrumRegisterHandler : Swift::GetResponder<Swift::InBandRegistrationPayload>, Swift::SetResponder<Swift::InBandRegistrationPayload> {
	public:
		SpectrumRegisterHandler(Swift::Component *component);
		~SpectrumRegisterHandler();

		// Registers new user, returns false if user was already registered.
		bool registerUser(const UserRow &row);

		// Unregisters user, returns true if user was successfully unregistered.
		bool unregisterUser(const std::string &barejid);

		void start();

	private:
		bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload);
		bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload);
		
		Swift::Component *m_component;

};

#endif
