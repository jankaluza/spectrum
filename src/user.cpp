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

#include "user.h"
#include "main.h"
#include "log.h"
#include "protocols/abstractprotocol.h"
#include "usermanager.h"
#include "gloox/chatstate.h"
#include "cmds.h"
#include "parser.h"
#include "proxy.h"
#include "filetransferrepeater.h"
#include "accountcollector.h"
#include "adhoc/adhochandler.h"
#include "sql.h"
#include "capabilityhandler.h"
#include "spectrumbuddy.h"
#include "transport.h"
#include "gloox/sha.h"
#include "Swiften/Swiften.h"
#include "spectrumpurple.h"

User::User(const UserRow &row, const std::string &userKey, Swift::Component *component, Swift::PresenceOracle *presenceOracle, Swift::EntityCapsManager *entityCapsManager, SpectrumPurple *instance) : SpectrumRosterManager(this, component), SpectrumMessageHandler(this, component) {
// 	p = parent;
	m_jid = row.jid;
	m_instance = instance;

	m_component = component;
	m_presenceOracle = presenceOracle;
	m_entityCapsManager = entityCapsManager;
	m_activeResource = Swift::JID(m_jid.c_str()).getResource().getUTF8String();
// 	Resrource r;
// 	setResource(Swift::JID(m_jid.c_str()).getResource().getUTF8String());
// 	setActiveResource(Swift::JID(m_jid.c_str()).getResource().getUTF8String());

// 	long id;
// 	std::string jid;
// 	std::string uin;
// 	std::string password;
// 	std::string language;
// 	std::string encoding;
// 	bool vip;
	
	m_userID = row.id;
	m_userKey = userKey;
	m_account = NULL;
	m_syncTimer = 0;
	m_subscribeLastCount = -1;
	m_vip = row.vip;
	m_readyForConnect = false;
	m_rosterXCalled = false;
	m_connected = false;
	m_reconnectCount = 0;
	m_glooxPresenceType = -1;

	m_password = row.password;
	m_username = row.uin;
	if (!CONFIG().username_mask.empty()) {
		std::string newUsername(CONFIG().username_mask);
		replace(newUsername, "$username", m_username.c_str());
		m_username = newUsername;
	}
	
	m_encoding = row.encoding;

	// There is some garbage in language column before 0.3 (this bug is fixed in 0.3), so we're trying to set default
	// values here.
	// TODO: Remove me for 0.4
	if (localization.getLanguages().find(row.language) == localization.getLanguages().end()) {
		UserRow res;
		res.jid = m_jid;
		res.password = m_password;
		if (row.language == "English") {
			res.language = "en";
			m_lang = g_strdup("en");
		}
		else {
			res.language = Transport::instance()->getConfiguration().language;
			m_lang = g_strdup(res.language.c_str());
		}
		res.encoding = m_encoding;
		Transport::instance()->sql()->updateUser(res);
	}
	else
		m_lang = g_strdup(row.language.c_str());

	m_features = 0;
	m_connectionStart = time(NULL);
	m_settings = Transport::instance()->sql()->getSettings(m_userID);

	m_loadingBuddiesFromDB = false;
	m_photoHash.clear();

	PurpleValue *value;

// 	PurpleAccount *act = purple_accounts_find(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str());
// 	if (act)
// 		Transport::instance()->collector()->stopCollecting(act);


	// check default settings
	if ( (value = getSetting("enable_transport")) == NULL ) {
		Transport::instance()->sql()->addSetting(m_userID, "enable_transport", "1", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, true);
		g_hash_table_replace(m_settings, g_strdup("enable_transport"), value);
	}
	if ( (value = getSetting("enable_notify_email")) == NULL ) {
		Transport::instance()->sql()->addSetting(m_userID, "enable_notify_email", "0", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, false);
		g_hash_table_replace(m_settings, g_strdup("enable_notify_email"), value);
	}
	if ( (value = getSetting("enable_avatars")) == NULL ) {
		Transport::instance()->sql()->addSetting(m_userID, "enable_avatars", "1", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, true);
		g_hash_table_replace(m_settings, g_strdup("enable_avatars"), value);
	}
	if ( (value = getSetting("enable_chatstate")) == NULL ) {
		Transport::instance()->sql()->addSetting(m_userID, "enable_chatstate", "1", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, true);
		g_hash_table_replace(m_settings, g_strdup("enable_chatstate"), value);
	}
	
	Transport::instance()->sql()->setUserOnline(m_userID, true);

	Transport::instance()->protocol()->onUserCreated(this);

// 	Tag *iq = new Tag("iq");
// 	iq->addAttribute("from", Transport::instance()->jid());
// 	iq->addAttribute("to", m_jid);
// 	iq->addAttribute("id", Transport::instance()->getId());
// 	iq->addAttribute("type", "set");
// 	Tag *query = new Tag("query");
// 	query->addAttribute("xmlns", "http://spectrum.im/protocol/remote-roster");
// 	iq->addChild(query);
// 	Transport::instance()->send(iq);
// 
// 	iq = new Tag("iq");
// 	iq->addAttribute("from", Transport::instance()->jid());
// 	iq->addAttribute("to", m_jid);
// 	iq->addAttribute("id", Transport::instance()->getId());
// 	iq->addAttribute("type", "get");
// 	query = new Tag("query");
// 	query->addAttribute("xmlns", "jabber:iq:roster");
// 	iq->addChild(query);
// 	Transport::instance()->send(iq);
}

/*
 * Returns true if transport has feature `feature` for this user,
 * otherwise returns false.
 */
bool User::hasTransportFeature(int feature) {
	if (m_features & feature)
		return true;
	return false;
}

PurpleValue * User::getSetting(const char *key) {
	PurpleValue *value = (PurpleValue *) g_hash_table_lookup(m_settings, key);
	return value;
}

void User::updateSetting(const std::string &key, PurpleValue *value) {
	if (purple_value_get_type(value) == PURPLE_TYPE_BOOLEAN) {
		if (purple_value_get_boolean(value))
			Transport::instance()->sql()->updateSetting(m_userID, key, "1");
		else
			Transport::instance()->sql()->updateSetting(m_userID, key, "0");
	}
	else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
		Transport::instance()->sql()->updateSetting(m_userID, key, purple_value_get_string(value));
	}
	g_hash_table_replace(m_settings, g_strdup(key.c_str()), purple_value_dup(value));
}

/*
 * Called when legacy network user stops typing.
 */
void User::purpleBuddyTypingStopped(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	if (!purple_value_get_boolean(getSetting("enable_chatstate")))
		return;
	Log(m_jid, uin << " stopped typing");
	std::string username(uin);
	AbstractSpectrumBuddy *s_buddy = getRosterItem(uin);
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		username = JID::escapeNode(username);
	else
		std::for_each( username.begin(), username.end(), replaceBadJidCharacters() ); // OK

	Swift::Message::ref s(new Swift::Message());
	s->setTo(Swift::JID(m_jid));
	s->setFrom(Swift::JID(username + "@" + Transport::instance()->jid() + "/bot"));
	s->addPayload(boost::shared_ptr<Swift::ChatState>( new Swift::ChatState(Swift::ChatState::Active) ));
	m_component->sendMessage(s);
}

/*
 * Called when legacy network user starts typing.
 */
void User::purpleBuddyTyping(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	if (!purple_value_get_boolean(getSetting("enable_chatstate")))
		return;
	Log(m_jid, uin << " is typing");
	std::string username(uin);
	AbstractSpectrumBuddy *s_buddy = getRosterItem(uin);
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		username = JID::escapeNode(username);
	else
		std::for_each( username.begin(), username.end(), replaceBadJidCharacters() ); // OK

	Swift::Message::ref s(new Swift::Message());
	s->setTo(Swift::JID(m_jid));
	s->setFrom(Swift::JID(username + "@" + Transport::instance()->jid() + "/bot"));
	s->addPayload(boost::shared_ptr<Swift::ChatState>( new Swift::ChatState(Swift::ChatState::Composing) ));
	m_component->sendMessage(s);
}

/*
 * Called when legacy network user stops typing.
 */
void User::purpleBuddyTypingPaused(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	if (!purple_value_get_boolean(getSetting("enable_chatstate")))
		return;
	Log(m_jid, uin << " paused typing");
	std::string username(uin);
	AbstractSpectrumBuddy *s_buddy = getRosterItem(uin);
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		username = JID::escapeNode(username);
	else
		std::for_each( username.begin(), username.end(), replaceBadJidCharacters() ); // OK


	Swift::Message::ref s(new Swift::Message());
	s->setTo(Swift::JID(m_jid));
	s->setFrom(Swift::JID(username + "@" + Transport::instance()->jid() + "/bot"));
	s->addPayload(boost::shared_ptr<Swift::ChatState>( new Swift::ChatState(Swift::ChatState::Paused) ));
	m_component->sendMessage(s);
}

/*
 * Called when we're ready to connect (we know caps).
 */
void User::connect() {
	if (!m_readyForConnect) {
		Log(m_jid, "We are not ready for connect");
		return;
	}
	if (m_account) {
		Log(m_jid, "connect() has been called before");
		return;
	}
	
	// check if it's valid uin
	bool valid = false;
	if (!m_username.empty()) {
		valid = Transport::instance()->protocol()->isValidUsername(m_username);
	}
	if (!valid) {
		Log(m_jid, m_username << " This username is not valid, not connecting.");
		return;
	}

// 	if (purple_accounts_find(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str()) != NULL){
// 		Log(m_jid, "this account already exists");
// 		m_account = purple_accounts_find(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str());
// 		User *user = (User *) Transport::instance()->userManager()->getUserByAccount(m_account);
// 		if (user && user != this) {
// 			m_account = NULL;
// 			Log(m_jid, "This account is already connected by another jid " << user->jid());
// 			return;
// 		}
// 	}
// 	else {
// 		Log(m_jid, "creating new account");
// 		m_account = purple_account_new(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str());
// 
// 		purple_accounts_add(m_account);
// 	}
// 	Transport::instance()->collector()->stopCollecting(m_account);
// 
// 	PurplePlugin *plugin = purple_find_prpl(Transport::instance()->protocol()->protocol().c_str());
// 	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
// 	for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
// 		PurpleAccountOption *option = (PurpleAccountOption *) l->data;
// 		purple_account_remove_setting(m_account, purple_account_option_get_setting(option));
// 	}
// 
// 	std::map <std::string, PurpleAccountSettingValue> &settings = Transport::instance()->getConfiguration().purple_account_settings;
// 	for (std::map <std::string, PurpleAccountSettingValue>::iterator it = settings.begin(); it != settings.end(); it++) {
// 		PurpleAccountSettingValue v = (*it).second;
// 		std::string key((*it).first);
// 		switch (v.type) {
// 			case PURPLE_PREF_BOOLEAN:
// 				purple_account_set_bool(m_account, key.c_str(), v.b);
// 				break;
// 
// 			case PURPLE_PREF_INT:
// 				purple_account_set_int(m_account, key.c_str(), v.i);
// 				break;
// 
// 			case PURPLE_PREF_STRING:
// 				if (v.str)
// 					purple_account_set_string(m_account, key.c_str(), v.str);
// 				else
// 					purple_account_remove_setting(m_account, key.c_str());
// 				break;
// 
// 			case PURPLE_PREF_STRING_LIST:
// 				// TODO:
// 				break;
// 
// 			default:
// 				continue;
// 		}
// 	}
// 
// 	purple_account_set_string(m_account, "encoding", m_encoding.empty() ? Transport::instance()->getConfiguration().encoding.c_str() : m_encoding.c_str());
// 	purple_account_set_bool(m_account, "use_clientlogin", false);
// 	purple_account_set_bool(m_account, "require_tls",  Transport::instance()->getConfiguration().require_tls);
// 	purple_account_set_bool(m_account, "use_ssl",  Transport::instance()->getConfiguration().require_tls);
// 	purple_account_set_bool(m_account, "direct_connect", false);
// 	purple_account_set_bool(m_account, "check-mail", purple_value_get_boolean(getSetting("enable_notify_email")));
// 
// 	m_account->ui_data = this;
// 	
// 	Transport::instance()->protocol()->onPurpleAccountCreated(m_account);
// 
// 	m_loadingBuddiesFromDB = true;
// 	loadRoster();
// 	m_loadingBuddiesFromDB = false;
// 
// 	m_connectionStart = time(NULL);
// 	m_readyForConnect = false;
// 	purple_account_set_password(m_account,m_password.c_str());
// 	Log(m_jid, "UIN:" << m_username << " USER_ID:" << m_userID);
// 
// 	if (CONFIG().useProxy) {
// 		PurpleProxyInfo *info = purple_proxy_info_new();
// 		purple_proxy_info_set_type(info, PURPLE_PROXY_USE_ENVVAR);
// 		info->username = NULL;
// 		info->password = NULL;
// 		purple_account_set_proxy_info(m_account, info);
// 	}
// 
// 	if (valid && purple_value_get_boolean(getSetting("enable_transport"))) {
// 		purple_account_set_enabled(m_account, PURPLE_UI, TRUE);
// // 		purple_account_connect(m_account);
// 		const PurpleStatusType *statusType = purple_account_get_status_type_with_primitive(m_account, (PurpleStatusPrimitive) m_presenceType);
// 		if (statusType) {
// 			Log(m_jid, "Setting up default status.");
// 			if (!m_statusMessage.empty()) {
// 				purple_account_set_status(m_account, purple_status_type_get_id(statusType), TRUE, "message", m_statusMessage.c_str(), NULL);
// 			}
// 			else {
// 				purple_account_set_status(m_account, purple_status_type_get_id(statusType), TRUE, NULL);
// 			}
// 		}
// 	}
	m_instance->login(m_username.c_str(), m_password.c_str(), m_presenceType, m_statusMessage.c_str());

	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(Swift::JID(m_jid));
	response->setFrom(Swift::JID(Transport::instance()->jid()));
	response->setType(Swift::Presence::Unavailable);
	response->setStatus(tr(getLang(), _("Connecting")));

	m_component->sendPresence(response);
}

Swift::Presence::ref User::findResourceWithFeature(const std::string &feature) {
	Swift::Presence::ref presence;
	// TODO: Iterate it somehow !!!!!!!!!!!!!!
	Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(Swift::JID(m_jid));
	if (!highest)
		return Swift::Presence::ref();
	
	Swift::DiscoInfo::ref caps = m_entityCapsManager->getCaps(highest->getFrom());
	if (!caps)
		return Swift::Presence::ref();
	
	if (caps->hasFeature(feature))
		return Swift::Presence::ref();
	
	return highest;
}

/*
 * called when we are disconnected from legacy network
 */
void User::disconnected() {
	m_connected = false;
	if (m_account) {
		m_account->ui_data = NULL;
	}
	m_account = NULL;
	m_readyForConnect = true;
	m_reconnectCount += 1;
}

/*
 * called when we are disconnected from legacy network
 */
void User::connected() {
	m_connected = true;
	m_reconnectCount = 0;
	Transport::instance()->protocol()->onConnected(this);

	std::cout << "CONNECTED\n";
	for (std::list <Tag*>::iterator it = m_autoConnectRooms.begin(); it != m_autoConnectRooms.end() ; it++ ) {
		Tag *stanza = (*it);
		GHashTable *comps = NULL;
		std::string name = "";
		std::string nickname = JID(stanza->findAttribute("to")).resource();

		PurpleConnection *gc = purple_account_get_connection(m_account);

		Transport::instance()->protocol()->makePurpleUsernameRoom(this, JID(stanza->findAttribute("to")).bare(), name);
		PurpleChat *chat = Transport::instance()->protocol()->getPurpleChat(this, name);
		if (chat) {
			comps = purple_chat_get_components(chat);
		}
		else if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
			comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
		}
		std::cout << name << " JOINING\n";
		if (comps) {
			setRoomResource(name, JID(stanza->findAttribute("from")).resource());
			serv_join_chat(gc, comps);
			g_hash_table_destroy(comps);
		}
		delete (*it);
	};

	m_autoConnectRooms.clear();

	
	Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(Swift::JID(m_jid));
	forwardStatus(highest);
	
// 	if (m_glooxPresenceType != -1) {
// 		Presence tag((Presence::PresenceType) m_glooxPresenceType, m_jid, m_statusMessage);
// 		tag.setFrom(Transport::instance()->jid());
// 		Transport::instance()->send(tag.tag());
// 		m_glooxPresenceType = -1;
// 	}
}

/*
 * Received jabber presence...
 */
void User::receivedPresence(Swift::Presence::ref presence) {
	bool statusChanged = false;

// 	if (stanza.to().username() == "" && isConnected() && stanza.presence() != Presence::Unavailable) {
// 		Tag *x_vcard = stanzaTag->findChild("x");
// 		if (x_vcard) {
// 			Tag *photo = x_vcard->findChild("photo");
// 			if (photo && !photo->cdata().empty()) {
// 				if (photo->cdata() != m_photoHash) {
// // 					Transport::instance()->fetchVCard(jid());
// 				}
// 			}
// 		}
// 	}
	
// 	handlePresence(stanza);


// 	// Handle join-the-room presence
// 	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
// 	if (isMUC && stanza.to().username() != "" && !isOpenedConversation(stanza.to().bare())) {
// 		if (stanza.presence() != Presence::Unavailable) {
// 			if (m_connected) {
// 				GHashTable *comps = NULL;
// 				std::string name = "";
// 				std::string nickname = stanza.to().resource();
// 
// 				PurpleConnection *gc = purple_account_get_connection(m_account);
// 				Transport::instance()->protocol()->makePurpleUsernameRoom(this, stanza.to().bare(), name);
// 				PurpleChat *chat = Transport::instance()->protocol()->getPurpleChat(this, name);
// 				if (chat) {
// 					comps = purple_chat_get_components(chat);
// 				}
// 				else if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
// 					comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
// 				}
// 				if (comps) {
// 					setRoomResource(name, stanza.from().resource());
// 					serv_join_chat(gc, comps);
// 				}
// 			}
// 			else {
// 				m_autoConnectRooms.push_back(stanza.tag());
// 			}
// 		}
// 	}

// 	std::cout << "Unavailable" << stanza.to().username() << "\n";
// 	if (stanza.to().username() != ""  && stanza.presence() == Presence::Unavailable) {
// 		std::string k = stanza.to().bare();
// 		removeConversation(k, true);
// 	}

	// this presence is for the transport
	if (presence->getTo().getNode().isEmpty()  || (Transport::instance()->protocol()->tempAccountsAllowed())) {
		if (presence->getType() == Swift::Presence::Unavailable) {
			// disconnect from legacy network if we are connected
			if (presence->getTo().getNode().isEmpty()) {
// // 				removeResource(stanza.from().resource());
				removeConversationResource(presence->getFrom().getResource().getUTF8String());
// 					T	ransport::instance()->adhoc()->unregisterSession(stanza.from().full()); TODO
				sendUnavailablePresenceToAll(presence->getFrom().getResource().getUTF8String());
			}
			if (m_connected) {
				Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(presence->getFrom().toBare());
				if ((!highest || (highest && highest->getType() == Swift::Presence::Unavailable)) || (Transport::instance()->protocol()->tempAccountsAllowed() && !hasOpenedMUC())){
					Log(m_jid, "disconecting");
// // 					sendUnavailablePresenceToAll();
// // 					purple_account_disconnect(m_account);
// 					Transport::instance()->adhoc()->unregisterSession(stanza.from().full()); TODO
				}
				else {
					
					// Active resource changed, so we probably want to update status message/show.
					forwardStatus(highest);
				}
// // 				m_connected=false;
			}
			else {
				Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(presence->getFrom().toBare());
				if (m_account && (!highest || (highest && highest->getType() == Swift::Presence::Unavailable))) {
					sendUnavailablePresenceToAll();
				}
// 				if (!getResources().empty()) {
// 					setActiveResource();
// 				}
// 				else if (m_account) {
// 					Log(m_jid, "disconecting2");
// 					sendUnavailablePresenceToAll();
// // 					purple_account_disconnect(m_account);
// 				}
			}
			// move me to presenceoracle
			std::string resource = presence->getFrom().getResource().getUTF8String();
			m_resources.erase(resource);
		} else {
			// move me to presenceoracle
			std::string resource = presence->getFrom().getResource().getUTF8String();
			if (m_resources.find(resource) == m_resources.end()) {
				m_resources[resource] = 1;
				sendPresenceToAll(presence->getFrom().toString().getUTF8String());
			}

			Swift::StatusShow::Type presenceShow = presence->getShow();
			std::string stanzaStatus = presence->getStatus().getUTF8String();
				
			// Active resource has been changed, so we have to inform SpectrumMessageHandler to send
			// next message from legacy network to bare jid.
			Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(presence->getFrom().toBare());
			if (highest->getFrom().getResource().getUTF8String() != m_activeResource) {
				removeConversationResource(m_activeResource);
				m_activeResource = highest->getFrom().getResource().getUTF8String();
				presenceShow = highest->getShow();
				stanzaStatus = highest->getStatus().getUTF8String();
			}

// 			Log(m_jid, "RESOURCE" << getResource().name << " " << stanza.from().resource());
			if (!m_connected) {
				statusChanged = true;
				// we are not connected to legacy network, so we should do it when disco#info arrive :)
				Log(m_jid, "connecting: resource=" << getResource().name);
				if (m_readyForConnect == false) {
					m_readyForConnect = true;
					// Forward status message to legacy network, but only if it's sent from active resource
// 					if (m_activeResource == presence->getFrom().getResource().getUTF8String()) {
// 						forwardStatus(presenceShow, stanzaStatus);
// 					}
					boost::shared_ptr<Swift::CapsInfo> capsInfo = presence->getPayload<Swift::CapsInfo>();
					if (capsInfo && capsInfo->getHash() == "sha-1") {
						if (m_entityCapsManager->getCaps(presence->getFrom()) != Swift::DiscoInfo::ref()) {
							connect();
						}
					}
					else {
						connect();
					}
// 					m_entityCapsManager->getCaps()
// 					if (getResource().caps == -1) {
// 						// caps not arrived yet, so we can't connect just now and we have to wait for caps
// 					}
// 					else {
// 						connect();
// 					}
				}
			}
			// Forward status message to legacy network, but only if it's sent from active resource
			else if (m_activeResource == presence->getFrom().getResource().getUTF8String()) {
				Log(m_jid, "mirroring presence to legacy network");
				statusChanged = true;
				forwardStatus(highest);
			}
		}

// 		if (purple_value_get_boolean(getSetting("enable_transport")) == false) {
// 			Tag *tag = new Tag("presence");
// 			tag->addAttribute("to", stanza.from().bare());
// 			tag->addAttribute("type", "unavailable");
// 			tag->addAttribute("from", Transport::instance()->jid());
// 			Transport::instance()->send(tag);
// 		}
// 		// send presence about tranport status to user
// 		else if (getResources().empty()) {
// 			Tag *tag = new Tag("presence");
// 			tag->addAttribute("to", stanza.from().bare());
// 			tag->addAttribute("type", "unavailable");
// 			tag->addAttribute("from", Transport::instance()->jid());
// 			Transport::instance()->send(tag);
// 		}
// 		else if (stanza.presence() == Presence::Unavailable) {
// 			Presence tag(stanza.presence(), stanza.from().full(), stanza.status());
// 			tag.setFrom(Transport::instance()->jid());
// 			Transport::instance()->send(tag.tag());
// 		}
// // 		else if (statusChanged) {
// // 			Presence tag(stanza.presence(), m_jid, stanza.status());
// // 			tag.setFrom(Transport::instance()->jid());
// // 			Transport::instance()->send(tag);
// // 		}

	}
// 	delete stanzaTag;
}

void User::forwardStatus(Swift::Presence::ref presence) {
	int PurplePresenceType;
	// mirror presence types
	switch (presence->getShow()) {
		case Swift::StatusShow::Online: {
			PurplePresenceType = PURPLE_STATUS_AVAILABLE;
			break;
		}
		case Swift::StatusShow::FFC: {
			PurplePresenceType = PURPLE_STATUS_AVAILABLE;
			break;
		}
		case Swift::StatusShow::Away: {
			PurplePresenceType = PURPLE_STATUS_AWAY;
			break;
		}
		case Swift::StatusShow::DND: {
			PurplePresenceType = PURPLE_STATUS_UNAVAILABLE;
			break;
		}
		case Swift::StatusShow::XA: {
			PurplePresenceType = PURPLE_STATUS_EXTENDED_AWAY;
			break;
		}
		default:
			PurplePresenceType = PURPLE_STATUS_AVAILABLE;
			break;
	}

	if (m_connected) {
		// we are already connected so we have to change status
		m_instance->changeStatus(m_username, PurplePresenceType, presence->getStatus().getUTF8String());

		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(presence->getFrom().toBare());
		response->setFrom(presence->getTo());
		response->setType(presence->getType());
		response->setShow(presence->getShow());
		response->setStatus(presence->getStatus());

		m_component->sendPresence(response);
	}
}

/* XXX: This needs to handle conversion to the right size and format,
 * per the PurplePluginProtocolInfo::icon_spec spec.
 */
void User::handleVCard(const VCard* vcard) {
	Log("handleVCard", "setting account icon for " << m_jid);
	gssize size = vcard->photo().binval.size();
	// this will be freed by libpurple
	guchar *photo = (guchar *) g_malloc(size * sizeof(guchar));
	memcpy(photo, vcard->photo().binval.c_str(), size);
	purple_buddy_icons_set_account_icon(m_account, photo, size);

	SHA sha;
	sha.feed( vcard->photo().binval );
	m_photoHash = sha.hex();
	Log("handleVCard", "new photoHash is " << m_photoHash);
}

User::~User(){
	Log("User Destructor", m_jid << " " << m_account << " " << (m_account ? purple_account_get_username(m_account) : "") );
	Transport::instance()->protocol()->onDestroy(this);
	g_free(m_lang);

	sendUnavailablePresenceToAll();

	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(Swift::JID(m_jid));
	response->setFrom(Swift::JID(Transport::instance()->jid()));
	response->setType(Swift::Presence::Unavailable);

	m_component->sendPresence(response);

	m_instance->logout(m_username);


	if (m_syncTimer != 0) {
		Log(m_jid, "removing timer\n");
		purple_timeout_remove(m_syncTimer);
	}

	g_hash_table_destroy(m_settings);
}


