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

#include "purple.h"
#include <gloox/iqhandler.h>
#include <gloox/stanzaextension.h>

using namespace gloox;
struct UserRow;

class RegisterExtension : public StanzaExtension
{

public:
    RegisterExtension();
    RegisterExtension(const Tag *tag);
    virtual ~RegisterExtension();
    virtual const std::string& filterString() const;
    virtual StanzaExtension* newInstance( const Tag* tag ) const
    {
        return new RegisterExtension(tag);
    }
    virtual Tag* tag() const;
    virtual StanzaExtension* clone() const
    {
        return new RegisterExtension(m_tag);
    }
private:
    Tag *m_tag;
};

class GlooxRegisterHandler : public IqHandler {
	public:
		GlooxRegisterHandler();
		~GlooxRegisterHandler();

		static GlooxRegisterHandler *instance() { return m_pInstance; }

		bool handleIq (const IQ &iq);
		bool handleIq (const Tag *iqTag);
		void handleIqID (const IQ &iq, int context);

		bool registerUser(const UserRow &row);

	private:
		void sendError(int code, const std::string &error, const Tag *iqTag);

		static GlooxRegisterHandler *m_pInstance;
};

#endif
