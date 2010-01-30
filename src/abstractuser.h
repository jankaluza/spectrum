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

#ifndef _ABSTRACT_USER_H
#define _ABSTRACT_USER_H

#include <string>
#include <list>
#include "account.h"
#include "value.h"
#include "resourcemanager.h"
#ifndef TESTS
#include "filetransferrepeater.h"
#endif

class AbstractUser : public ResourceManager
{
	public:
		AbstractUser() { m_filetransfers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL); }
		virtual ~AbstractUser() { g_hash_table_destroy(m_filetransfers); }
		virtual const std::string &userKey() = 0;
		virtual long storageId() = 0;
		virtual PurpleAccount *account() = 0;
		virtual const std::string &jid() = 0;
		virtual PurpleValue *getSetting(const char *key) = 0;
		virtual bool hasTransportFeature(int feature) = 0;
		virtual int getFeatures() = 0;
		virtual const std::string &username() = 0;
		const char *getLang() { return "en"; }
		virtual GHashTable *settings() = 0;
		virtual bool isConnectedInRoom(const std::string &room) = 0;
		virtual bool isConnected() = 0;
		virtual bool readyForConnect() = 0;
		virtual void connect() = 0;
		virtual void disconnected() = 0;
		virtual void updateSetting(const std::string &key, PurpleValue *value) {}
		
		void setProtocolData(void *protocolData) { m_protocolData = protocolData; }
		void *protocolData() { return m_protocolData; }
		std::string getRoomResource(const std::string &room) { return m_roomResources[room]; }
		void setRoomResource(const std::string &room, const std::string &resource) { m_roomResources[room] = resource; }

#ifndef TESTS
		void addFiletransfer( const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size ) {
			FiletransferRepeater *ft = new FiletransferRepeater(to, sid, type, from, size);
			g_hash_table_replace(m_filetransfers, g_strdup(to.bare() == jid() ? from.username().c_str() : to.username().c_str()), ft);
		}
		void addFiletransfer( const JID& to ) {
			FiletransferRepeater *ft = new FiletransferRepeater(to, jid() + "/" + getResource().name);
			g_hash_table_replace(m_filetransfers, g_strdup(to.username().c_str()), ft);
		}
		FiletransferRepeater* removeFiletransfer(const std::string &from) {
			FiletransferRepeater *repeater = (FiletransferRepeater *) g_hash_table_lookup(m_filetransfers, from.c_str()); if (repeater) g_hash_table_remove(m_filetransfers, from.c_str()); return repeater;
			return NULL;
		}
		FiletransferRepeater* getFiletransfer(const std::string &from) {
			FiletransferRepeater *repeater = (FiletransferRepeater *) g_hash_table_lookup(m_filetransfers, from.c_str()); return repeater;
		}
#endif
		guint removeTimer;

	private:
		void *m_protocolData;
		std::map<std::string, std::string> m_roomResources;
		GHashTable *m_filetransfers;
		
};

#endif
