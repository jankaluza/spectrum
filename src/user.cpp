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

User::User(GlooxMessageHandler *parent, JID jid, const std::string &username, const std::string &password, const std::string &userKey, long id, const std::string &encoding, const std::string &language) : SpectrumRosterManager(this), SpectrumMessageHandler(this) {
	p = parent;
	m_jid = jid.bare();

	Resource r;
	setResource(jid.resource());
	setActiveResource(jid.resource());

	m_userID = id;
	m_userKey = userKey;
	m_account = NULL;
	m_syncTimer = 0;
	m_subscribeLastCount = -1;
	m_vip = p->sql()->isVIP(m_jid);
	m_readyForConnect = false;
	m_rosterXCalled = false;
	m_connected = false;
	m_reconnectCount = 0;

	m_password = password;
	m_username = username;
	m_encoding = encoding;

	m_lang = g_strdup(language.c_str());
	m_features = 0;
	m_connectionStart = time(NULL);
	m_settings = p->sql()->getSettings(m_userID);

	m_loadingBuddiesFromDB = false;
	m_photoHash.clear();

	PurpleValue *value;

	PurpleAccount *act = purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str());
	if (act)
		p->collector()->stopCollecting(act);


	// check default settings
	if ( (value = getSetting("enable_transport")) == NULL ) {
		p->sql()->addSetting(m_userID, "enable_transport", "1", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, true);
		g_hash_table_replace(m_settings, g_strdup("enable_transport"), value);
	}
	if ( (value = getSetting("enable_notify_email")) == NULL ) {
		p->sql()->addSetting(m_userID, "enable_notify_email", "0", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, false);
		g_hash_table_replace(m_settings, g_strdup("enable_notify_email"), value);
	}
	if ( (value = getSetting("enable_avatars")) == NULL ) {
		p->sql()->addSetting(m_userID, "enable_avatars", "1", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, true);
		g_hash_table_replace(m_settings, g_strdup("enable_avatars"), value);
	}
	if ( (value = getSetting("enable_chatstate")) == NULL ) {
		p->sql()->addSetting(m_userID, "enable_chatstate", "1", PURPLE_TYPE_BOOLEAN);
		value = purple_value_new(PURPLE_TYPE_BOOLEAN);
		purple_value_set_boolean(value, true);
		g_hash_table_replace(m_settings, g_strdup("enable_chatstate"), value);
	}
	
	Transport::instance()->sql()->setUserOnline(m_userID, true);

	p->protocol()->onUserCreated(this);
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
			p->sql()->updateSetting(m_userID, key, "1");
		else
			p->sql()->updateSetting(m_userID, key, "0");
	}
	else if (purple_value_get_type(value) == PURPLE_TYPE_STRING) {
		p->sql()->updateSetting(m_userID, key, purple_value_get_string(value));
	}
	g_hash_table_replace(m_settings, g_strdup(key.c_str()), value);
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
	std::for_each( username.begin(), username.end(), replaceBadJidCharacters() );


	Tag *s = new Tag("message");
	s->addAttribute("to",m_jid);
	s->addAttribute("type","chat");
	s->addAttribute("from",username + "@" + p->jid() + "/bot");

	// chatstates
	Tag *active = new Tag("active");
	active->addAttribute("xmlns","http://jabber.org/protocol/chatstates");
	s->addChild(active);

	p->j->send( s );
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
	std::for_each( username.begin(), username.end(), replaceBadJidCharacters() );

	Tag *s = new Tag("message");
	s->addAttribute("to", m_jid);
	s->addAttribute("type", "chat");
	s->addAttribute("from",username + "@" + p->jid() + "/bot");

	// chatstates
	Tag *active = new Tag("composing");
	active->addAttribute("xmlns","http://jabber.org/protocol/chatstates");
	s->addChild(active);

	p->j->send( s );
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
	std::for_each( username.begin(), username.end(), replaceBadJidCharacters() );


	Tag *s = new Tag("message");
	s->addAttribute("to",m_jid);
	s->addAttribute("type","chat");
	s->addAttribute("from",username + "@" + p->jid() + "/bot");

	// chatstates
	Tag *active = new Tag("paused");
	active->addAttribute("xmlns","http://jabber.org/protocol/chatstates");
	s->addChild(active);

	p->j->send( s );
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
		valid = p->protocol()->isValidUsername(m_username);
	}
	if (!valid) {
		Log(m_jid, m_username << " This username is not valid, not connecting.");
		return;
	}

	if (purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str()) != NULL){
		Log(m_jid, "this account already exists");
		m_account = purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str());
		User *user = (User *) Transport::instance()->userManager()->getUserByAccount(m_account);
		if (user && user != this) {
			Log(m_jid, "This account is already connected by another jid " << user->jid());
			return;
		}
	}
	else {
		Log(m_jid, "creating new account");
		m_account = purple_account_new(m_username.c_str(), this->p->protocol()->protocol().c_str());

		purple_accounts_add(m_account);
	}
/*
	PurplePlugin *plugin = purple_find_prpl(Transport::instance()->protocol()->protocol().c_str());
	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
	for (GList *l = prpl_info->protocol_options; l != NULL; l = l->next) {
		PurpleAccountOption *option = (PurpleAccountOption *) l->data;
		purple_account_remove_setting(m_account, purple_account_option_get_setting(option));
	}

	std::map <std::string, PurpleAccountSettingValue> &settings = Transport::instance()->getConfiguration().purple_account_settings;
	for (std::map <std::string, PurpleAccountSettingValue>::iterator it = settings.begin(); it != settings.end(); it++) {
		PurpleAccountSettingValue v = (*it).second;
		std::string key((*it).first);
		switch (v.type) {
			case PURPLE_PREF_BOOLEAN:
				purple_account_set_bool(m_account, key.c_str(), v.b);
				break;

			case PURPLE_PREF_INT:
				purple_account_set_int(m_account, key.c_str(), v.i);
				break;

			case PURPLE_PREF_STRING:
				if (v.str)
					purple_account_set_string(m_account, key.c_str(), v.str);
				else
					purple_account_remove_setting(m_account, key.c_str());
				break;

			case PURPLE_PREF_STRING_LIST:
				// TODO:
				break;

			default:
				continue;
		}
	}*/

	purple_account_set_string(m_account, "encoding", m_encoding.empty() ? Transport::instance()->getConfiguration().encoding.c_str() : m_encoding.c_str());
	purple_account_set_bool(m_account, "use_clientlogin", false);
	purple_account_set_bool(m_account, "require_tls",  Transport::instance()->getConfiguration().require_tls);
	purple_account_set_bool(m_account, "use_ssl",  Transport::instance()->getConfiguration().require_tls);

	m_account->ui_data = this;
	
	p->protocol()->onPurpleAccountCreated(m_account);

	m_loadingBuddiesFromDB = true;
	loadRoster();
	m_loadingBuddiesFromDB = false;

	m_connectionStart = time(NULL);
	m_readyForConnect = false;
	purple_account_set_password(m_account,m_password.c_str());
	Log(m_jid, "UIN:" << m_username << " USER_ID:" << m_userID);

	if (p->configuration().useProxy) {
		PurpleProxyInfo *info = purple_proxy_info_new();
		purple_proxy_info_set_type(info, PURPLE_PROXY_USE_ENVVAR);
		info->username = NULL;
		info->password = NULL;
		purple_account_set_proxy_info(m_account, info);
	}

	if (valid && purple_value_get_boolean(getSetting("enable_transport"))) {
		purple_account_set_enabled(m_account, PURPLE_UI, TRUE);
		purple_account_connect(m_account);
	}
}

/*
 * called when we are disconnected from legacy network
 */
void User::disconnected() {
	m_connected = false;
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
	p->protocol()->onConnected(this);
}

/*
 * Received jabber presence...
 */
void User::receivedPresence(const Presence &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag)
		return;

// 	std::string lang = stanzaTag->findAttribute("xml:lang");
// 	if (lang != "") {
// 		setLang(lang.c_str());
// 		localization.loadLocale(getLang());
// 	}
	
	if (stanza.to().username() == "" && isConnected()) {
		Tag *x_vcard = stanzaTag->findChild("x");
		if (x_vcard) {
			Tag *photo = x_vcard->findChild("photo");
			if (photo && !photo->cdata().empty()) {
				if (photo->cdata() != m_photoHash) {
					p->fetchVCard(jid());
				}
			}
		}
	}
	
	handlePresence(stanza);

	if (p->protocol()->onPresenceReceived(this, stanza)) {
		delete stanzaTag;
		return;
	}

	if (stanza.to().username() != ""  && p->protocol()->isMUC(NULL, stanza.to().bare()) && stanza.presence() == Presence::Unavailable) {
		removeConversation(stanza.to().username());
	}

	// this presence is for the transport
	if (stanza.to().username() == ""  || ((p->protocol()->tempAccountsAllowed()) && p->protocol()->isMUC(NULL, stanza.to().bare()))) {
		if (stanza.presence() == Presence::Unavailable) {
			// disconnect from legacy network if we are connected
			if (stanza.to().username() == "") {
// 				if ((m_connected == false && int(time(NULL)) > int(m_connectionStart) + 10) || m_connected == true) {
					if (hasResource(stanza.from().resource())) {
						removeResource(stanza.from().resource());
						removeConversationResource(stanza.from().resource());
						p->adhoc()->unregisterSession(stanza.from().full());
					}
					sendUnavailablePresenceToAll(stanza.from().resource());
// 				}
			}
			if (m_connected) {
				if (getResources().empty() || (p->protocol()->tempAccountsAllowed() && !hasOpenedMUC())){
					Log(m_jid, "disconecting");
					sendUnavailablePresenceToAll();
					purple_account_disconnect(m_account);
					p->adhoc()->unregisterSession(stanza.from().full());
					p->userManager()->removeUserTimer(this);
				}
				else {
					setActiveResource();
				}
// 				m_connected=false;
			}
			else {
				if (!getResources().empty()) {
					setActiveResource();
				}
				else if (m_account) {
					Log(m_jid, "disconecting2");
					sendUnavailablePresenceToAll();
					purple_account_disconnect(m_account);
				}
			}
		} else {
			if (!hasResource(stanza.from().resource())) {
				setResource(stanza); // add this resource
				sendPresenceToAll(stanza.from().full());
			}
			else
				setResource(stanza); // update this resource

			Log(m_jid, "resource: " << getResource().name);
			if (!m_connected) {
				// we are not connected to legacy network, so we should do it when disco#info arrive :)
				Log(m_jid, "connecting: resource=" << getResource().name);
				if (m_readyForConnect == false) {
					m_readyForConnect = true;
					if (getResource().caps == -1) {
						// caps not arrived yet, so we can't connect just now and we have to wait for caps
					}
					else {
						connect();
					}
				}
			}
			else {
				Log(m_jid, "mirroring presence to legacy network");
				// we are already connected so we have to change status
				int PurplePresenceType;
				const PurpleStatusType *status_type;
				std::string statusMessage;

				// mirror presence types
				switch (stanza.presence()) {
					case Presence::Available: {
						PurplePresenceType = PURPLE_STATUS_AVAILABLE;
						break;
					}
					case Presence::Chat: {
						PurplePresenceType = PURPLE_STATUS_AVAILABLE;
						break;
					}
					case Presence::Away: {
						PurplePresenceType = PURPLE_STATUS_AWAY;
						break;
					}
					case Presence::DND: {
						PurplePresenceType = PURPLE_STATUS_UNAVAILABLE;
						break;
					}
					case Presence::XA: {
						PurplePresenceType = PURPLE_STATUS_EXTENDED_AWAY;
						break;
					}
					default: break;
				}

				status_type = purple_account_get_status_type_with_primitive(m_account, (PurpleStatusPrimitive) PurplePresenceType);
				if (status_type != NULL) {
					// send presence to legacy network
					statusMessage.clear();

					statusMessage.append(stanza.status());

					if (!statusMessage.empty()) {
						purple_account_set_status(m_account, purple_status_type_get_id(status_type), TRUE, "message", statusMessage.c_str(), NULL);
					}
					else {
						purple_account_set_status(m_account, purple_status_type_get_id(status_type), TRUE, NULL);
					}
				}
			}
		}
		
		if (purple_value_get_boolean(getSetting("enable_transport")) == false) {
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", stanza.from().bare());
			tag->addAttribute("type", "unavailable");
			tag->addAttribute("from", p->jid());
			p->j->send(tag);
		}
		// send presence about tranport status to user
		else if (m_connected || m_readyForConnect) {
			if (stanza.presence() == Presence::Unavailable) {
				Presence tag(stanza.presence(), stanza.from().full(), stanza.status());
				tag.setFrom(p->jid());
				p->j->send(tag);
			}
			else {
				Presence tag(stanza.presence(), m_jid, stanza.status());
				tag.setFrom(p->jid());
				p->j->send(tag);
			}
		} else {
			if (getResources().empty()) {
				Tag *tag = new Tag("presence");
				tag->addAttribute("to", stanza.from().bare());
				tag->addAttribute("type", "unavailable");
				tag->addAttribute("from", p->jid());
				p->j->send(tag);
			}
		}
	}
	delete stanzaTag;
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
	g_free(m_lang);
	if (m_account)
		purple_account_set_enabled(m_account, PURPLE_UI, FALSE);

	sendUnavailablePresenceToAll();

	GList *iter;
	for (iter = purple_get_conversations(); iter; ) {
		PurpleConversation *conv = (PurpleConversation*) iter->data;
		iter = iter->next;
		if (purple_conversation_get_account(conv) == m_account)
			purple_conversation_destroy(conv);
	}

	// purple_account_destroy(m_account);
	// delete(m_account);
	// if (this->save_timer!=0 && this->save_timer!=-1)
	// 	std::cout << "* removing timer\n";
	// 	purple_timeout_remove(this->save_timer);
	if (m_account) {
		m_account->ui_data = NULL;
		p->collector()->collect(m_account);
	}
	else {
		PurpleAccount *act = purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str());
		if (act) {
			act->ui_data = NULL;
			p->collector()->collect(act);
			purple_account_disconnect(act);
			purple_account_set_enabled(act, PURPLE_UI, FALSE);
		}
	}

	if (m_syncTimer != 0) {
		Log(m_jid, "removing timer\n");
		purple_timeout_remove(m_syncTimer);
	}

	g_hash_table_destroy(m_settings);

	p->protocol()->onDestroy(this);
}


