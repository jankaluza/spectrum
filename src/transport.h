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

#ifndef TRANSPORT_H
#define TRANSPORT_H
#include <iostream>
#include <string>
#include "gloox/tag.h"

#include "abstractbackend.h"


using namespace gloox;
class UserManager;

/*
 * Transport features used to configure transport.
 */
typedef enum { 	TRANSPORT_FEATURE_TYPING_NOTIFY = 2,
				TRANSPORT_FEATURE_AVATARS = 4,
                TRANSPORT_MANGLE_STATUS = 8,
				TRANSPORT_FEATURE_FILETRANSFER = 16
				} TransportFeatures;


class Transport {
	public:
		Transport(const std::string jid);
		~Transport();
		static Transport *instance() { return m_pInstance; }
		void send(Tag *tag);
		UserManager *userManager();
		const std::string &hash();
		AbstractBackend *sql();
		const std::string &jid() { return m_jid; }
		std::string getId();
		int getFeatures(const std::string &ver);
		std::list <Tag *> &getTags() { return m_tags; }
		void clearTags() { m_tags.clear(); }
		
		
		
	private:
		std::string m_jid;
		std::string m_hash;
		std::list <Tag *> m_tags;
		UserManager *m_userManager;
		static Transport *m_pInstance;
};

#endif
