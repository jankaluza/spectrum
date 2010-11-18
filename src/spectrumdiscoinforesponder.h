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

#ifndef SPECTRUM_DISCO_INFO_RESPONDER_H
#define SPECTRUM_DISCO_INFO_RESPONDER_H

#include <vector>
#include "Swiften/Swiften.h"
#include "Swiften/Queries/GetResponder.h"
#include "Swiften/Elements/DiscoInfo.h"

class SpectrumDiscoInfoResponder : public Swift::GetResponder<Swift::DiscoInfo> {
	public:
		SpectrumDiscoInfoResponder(Swift::IQRouter *router);
		~SpectrumDiscoInfoResponder();

	private:
		virtual bool handleGetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::DiscoInfo> payload);

		Swift::DiscoInfo m_transportInfo;
		Swift::DiscoInfo m_buddyInfo;
};

#endif
