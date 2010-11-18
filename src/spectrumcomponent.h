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

#ifndef SPECTRUM_COMPONENT_H
#define SPECTRUM_COMPONENT_H

#include <vector>
#include "Swiften/Swiften.h"
#include "Swiften/Disco/GetDiscoInfoRequest.h"
#include "Swiften/Disco/EntityCapsManager.h"
#include "Swiften/Disco/CapsManager.h"
#include "Swiften/Disco/CapsMemoryStorage.h"
#include "Swiften/Presence/PresenceOracle.h"
#include "Swiften/Network/BoostTimerFactory.h"
#include "Swiften/Network/BoostIOServiceThread.h"
#include "glib.h"

typedef enum { 	CLIENT_FEATURE_ROSTERX = 2,
				CLIENT_FEATURE_XHTML_IM = 4,
				CLIENT_FEATURE_FILETRANSFER = 8,
				CLIENT_FEATURE_CHATSTATES = 16
				} SpectrumImportantFeatures;

class SpectrumTimer;
class SpectrumDiscoInfoResponder;

class SpectrumComponent {
	public:
		SpectrumComponent(Swift::EventLoop *loop);
		~SpectrumComponent();

		// Connect to server
		void connect();
	
	private:
		void handleConnected();
		void handleConnectionError(const Swift::ComponentError &error);
		void handlePresenceReceived(Swift::Presence::ref presence);
		void handleMessageReceived(Swift::Message::ref message);
		void handlePresence(Swift::Presence::ref presence);
		void handleSubscription(Swift::Presence::ref presence);
		void handleProbePresence(Swift::Presence::ref presence);
		void handleDataRead(const Swift::String &data);
		void handleDataWritten(const Swift::String &data);
		
		void handleDiscoInfoResponse(boost::shared_ptr<Swift::DiscoInfo> info, const boost::optional<Swift::ErrorPayload>& error, const Swift::JID& jid);
		void handleCapsChanged(const Swift::JID& jid);

		Swift::BoostNetworkFactories *m_factories;
		Swift::Component *m_component;
		Swift::Timer::ref m_reconnectTimer;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		Swift::EntityCapsManager *m_entityCapsManager;
		Swift::CapsManager *m_capsManager;
		Swift::CapsMemoryStorage *m_capsMemoryStorage;
		Swift::PresenceOracle *m_presenceOracle;
		SpectrumDiscoInfoResponder *m_discoInfoResponder;
		int m_reconnectCount;
		bool m_firstConnection;
};

#endif
