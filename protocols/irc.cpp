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

#include "irc.h"
#include "../main.h"
#include "../muchandler.h"
#include "../usermanager.h"

IRCProtocol::IRCProtocol(GlooxMessageHandler *main){
	m_main = main;
// 	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");
	m_transportFeatures.push_back("http://jabber.org/protocol/muc");
	
	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

}
	
IRCProtocol::~IRCProtocol() {}

std::list<std::string> IRCProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> IRCProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string IRCProtocol::text(const std::string &key) {
	if (key == "instructions")
		return "Enter your Facebook email and password:";
	return "not defined";
}

Tag *IRCProtocol::getVCardTag(User *user, GList *vcardEntries) {
// 	PurpleNotifyUserInfoEntry *vcardEntry;

	Tag *vcard = new Tag( "vCard" );
	vcard->addAttribute( "xmlns", "vcard-temp" );

	return vcard;
}

void IRCProtocol::onUserCreated(User *user) {
	PurpleValue *value;
	if ( (value = user->getSetting("nickserv")) == NULL ) {
		m_main->sql()->addSetting(user->userKey(), "nickserv", "", PURPLE_TYPE_STRING);
		value = purple_value_new(PURPLE_TYPE_STRING);
		purple_value_set_string(value, "");
		g_hash_table_replace(user->settings(), g_strdup("nickserv"), value);
	}
	user->setProtocolData(new IRCProtocolData());
}

void IRCProtocol::onConnected(User *user) {
	IRCProtocolData *data = (IRCProtocolData *) user->protocolData();
	const char *n = purple_value_get_string(user->getSetting("nickserv"));
	std::string nickserv(n ? n : "");
	if (!nickserv.empty()) {
		// receivedMessage will send PM according to resource, so it doesn't matter what's before it... :)
		Message msg(Message::Chat, JID("#test%test@server.cz/NickServ"), "identify " + nickserv);
		msg.setFrom(user->jid());
		user->receivedMessage(msg);
	}

	for (std::list <Tag*>::iterator it = data->autoConnectRooms.begin(); it != data->autoConnectRooms.end() ; it++ ) {
		Presence stanza((*it));
		MUCHandler *muc = new MUCHandler(user, stanza.to().bare(), stanza.from().full());
		g_hash_table_replace(user->mucs(), g_strdup(stanza.to().username().c_str()), muc);
		Tag * ret = muc->handlePresence(stanza);
		if (ret)
			m_main->j->send(ret);
		delete (*it);
	};
}

bool IRCProtocol::onPresenceReceived(User *user, const Presence &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (stanza.to().username()!="") {
		IRCProtocolData *data = (IRCProtocolData *) user->protocolData();
		GHashTable *m_mucs = user->mucs();
		MUCHandler *muc = (MUCHandler*) g_hash_table_lookup(m_mucs, stanza.to().username().c_str());
		if (muc) {
			Tag * ret = muc->handlePresence(stanza);
			if (ret)
				m_main->j->send(ret);
			if (stanza.presence() == Presence::Unavailable) {
				g_hash_table_remove(m_mucs, stanza.to().username().c_str());
				user->conversations().erase(stanza.to().username());
				delete muc;
			}
		}
		else if (isMUC(user, stanza.to().bare()) && stanza.presence() != Presence::Unavailable) {
			if (user->isConnected()) {
				MUCHandler *muc = new MUCHandler(user, stanza.to().bare(), stanza.from().full());
				g_hash_table_replace(m_mucs, g_strdup(stanza.to().username().c_str()), muc);
				Tag * ret = muc->handlePresence(stanza);
				if (ret)
					m_main->j->send(ret);
			}
			else {
				data->autoConnectRooms.push_back(stanza.tag());
			}
		}
	}
	
	// this presence is for the transport
	if ( !tempAccountsAllowed() || isMUC(NULL, stanza.to().bare()) ){
		if(stanza.presence() == Presence::Unavailable) {
			// disconnect from legacy network if we are connected
			if (user->isConnected()) {
				if (g_hash_table_size(user->mucs()) == 0) {
					Log().Get(user->jid()) << "disconecting";
					purple_account_disconnect(user->account());
					m_main->userManager()->removeUserTimer(user);
				}
// 				else {
// 					iter = m_resources.begin();
// 					m_resource=(*iter).first;
// 				}
			}
			else {
				if (!user->resources().empty() && int(time(NULL))>int(user->connectionStart())+10){
					std::map<std::string,Resource> ::iterator iter = user->resources().begin();
					iter = user->resources().begin();
					std::string res = (*iter).first;
					user->setActiveResource(res);
				}
				else if (user->account()){
					Log().Get(user->jid()) << "disconecting2";
					purple_account_disconnect(user->account());
				}
			}
		} else {
			std::string resource = stanza.from().resource();
			std::map<std::string,Resource> ::iterator iter = user->resources().begin();
			iter = user->resources().find(resource);
			if(iter == user->resources().end()){
				user->resources()[resource].priority = stanza.priority();
				Tag *c = stanzaTag->findChildWithAttrib("xmlns","http://jabber.org/protocol/caps");
				// presence has caps
				if (c!=NULL){
					if (m_main->hasCaps(c->findAttribute("ver"))){
						user->resources()[resource].capsVersion = c->findAttribute("ver");
					}
				}
				if (user->resources()[resource].priority > user->resources()[user->resource()].priority)
					user->setActiveResource(resource);
			}

			Log().Get(user->jid()) << "resource: " << user->resource();
			if (!user->isConnected()) {
				// we are not connected to legacy network, so we should do it when disco#info arrive :)
				Log().Get(user->jid()) << "connecting: resource=" << user->resource();
				if (user->readyForConnect()==false){
					user->setReadyForConnect(true);
					if (user->getResource().capsVersion.empty()){
						// caps not arrived yet, so we can't connect just now and we have to wait for caps
					}
					else{
						user->connect();
					}
				}
			}
			else {
				Log().Get(user->jid()) << "mirroring presence to legacy network";
				// we are already connected so we have to change status
				PurpleSavedStatus *status;
				int PurplePresenceType;
				std::string statusMessage;
				
				// mirror presence types
				switch(stanza.presence()) {
					case Presence::Available: {
						PurplePresenceType=PURPLE_STATUS_AVAILABLE;
						break;
					}
					case Presence::Chat: {
						PurplePresenceType=PURPLE_STATUS_AVAILABLE;
						break;
					}
					case Presence::Away: {
						PurplePresenceType=PURPLE_STATUS_AWAY;
						break;
					}
					case Presence::DND: {
						PurplePresenceType=PURPLE_STATUS_UNAVAILABLE;
						break;
					}
					case Presence::XA: {
						PurplePresenceType=PURPLE_STATUS_EXTENDED_AWAY;
						break;
					}
					default: break;
				}
				// send presence to our legacy network
				status = purple_savedstatus_new(NULL, (PurpleStatusPrimitive)PurplePresenceType);

				statusMessage.clear();

				if (user->hasTransportFeature(TRANSPORT_MANGLE_STATUS)) {
					m_main->sql()->getRandomStatus(statusMessage);
                    statusMessage.append(" - ");
				}

				statusMessage.append(stanza.status());

				if (!statusMessage.empty())
					purple_savedstatus_set_message(status,statusMessage.c_str());
				purple_savedstatus_activate_for_account(status,user->account());
			}
		}
	}
	delete stanzaTag;
	return true;
}


void IRCProtocol::onDestroy(User *user) {
	IRCProtocolData *data = (IRCProtocolData *) user->protocolData();
	delete data;
}

