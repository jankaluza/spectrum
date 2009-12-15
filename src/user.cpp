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
#include "muchandler.h"
#include "cmds.h"
#include "parser.h"
#include "proxy.h"
#include "filetransferrepeater.h"
#include "accountcollector.h"
#include "striphtmltags.h"
#include "sql.h"
#include "caps.h"
#include "spectrumbuddy.h"
#include "transport.h"

User::User(GlooxMessageHandler *parent, JID jid, const std::string &username, const std::string &password, const std::string &userKey, long id) : RosterManager(this), RosterStorage(this), SpectrumMessageHandler(this) {
	p = parent;
	m_jid = jid.bare();
	m_userID = id;

	Resource r;
	setResource(jid.resource());
	setActiveResource(jid.resource());

	m_username = username;
	m_password = password;
	m_userKey = userKey;
	m_connected = false;
	m_vip = p->sql()->isVIP(m_jid);
	m_settings = p->sql()->getSettings(m_userID);
	m_syncTimer = 0;
	removeTimer = 0;
	m_readyForConnect = false;
	m_rosterXCalled = false;
	m_account = NULL;
	m_connectionStart = time(NULL);
	m_subscribeLastCount = -1;
	m_reconnectCount = 0;
	m_lang = NULL;
	m_features = 0;
	m_filetransfers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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

/*
 * Returns true if exists auth request from user with uin `name`.
 */
bool User::hasAuthRequest(const std::string &name){
	std::map<std::string,authRequest>::iterator iter = m_authRequests.begin();
	iter = m_authRequests.find(name);
	if(iter != m_authRequests.end())
		return true;
	return false;
}

void User::removeAuthRequest(const std::string &name) {
	m_authRequests.erase(name);
}

void User::purpleReauthorizeBuddy(PurpleBuddy *buddy) {
	if (!m_connected)
		return;
	if (!buddy)
		return;
	if (!m_account)
		return;
	GList *l, *ll;
	std::string name(purple_buddy_get_name(buddy));
	std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	if (purple_account_get_connection(m_account)) {
		PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_account_get_connection(m_account)->prpl);
		if (prpl_info && prpl_info->blist_node_menu) {
			for (l = ll = prpl_info->blist_node_menu((PurpleBlistNode*)buddy); l; l = l->next) {
				if (l->data) {
					PurpleMenuAction *act = (PurpleMenuAction *) l->data;
					if (act->label) {
						Log(m_jid, (std::string)act->label);
						if ((std::string) act->label == "Re-request Authorization") {
							Log(m_jid, "rerequesting authorization for " << name);
							((GSourceFunc) act->callback) (act->data);
							break;
						}
					}
					purple_menu_action_free(act);
				}
			}
			g_list_free(ll);
		}
	}
}

/*
 * Called when PurpleBuddy is removed.
 */
void User::purpleBuddyRemoved(PurpleBuddy *buddy) {
	handleBuddyRemoved(buddy);
	removeBuddy(buddy);
}

void User::purpleBuddyCreated(PurpleBuddy *buddy) {
	if (buddy==NULL || m_loadingBuddiesFromDB)
		return;
	buddy->node.ui_data = (void *) new SpectrumBuddy(-1, buddy);
	SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
	
	handleBuddyCreated(s_buddy);
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
	Tag *active = new Tag("paused");
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
 * Received Chatstate notification from jabber user :).
 */
void User::receivedChatState(const std::string &uin,const std::string &state){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	Log(m_jid, "Sending " << state << " message to " << uin);
	if (state == "composing")
		serv_send_typing(purple_account_get_connection(m_account),uin.c_str(),PURPLE_TYPING);
	else if (state == "paused")
		serv_send_typing(purple_account_get_connection(m_account),uin.c_str(),PURPLE_TYPED);
	else
		serv_send_typing(purple_account_get_connection(m_account),uin.c_str(),PURPLE_NOT_TYPING);
}

/*
 * Somebody wants to authorize us from the legacy network.
 */
void User::purpleAuthorizeReceived(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data){
	authRequest req;
	req.authorize_cb = authorize_cb;
	req.deny_cb = deny_cb;
	req.user_data = user_data;
	m_authRequests[std::string(remote_user)] = req;
	Log(m_jid, "purpleAuthorizeReceived: " << std::string(remote_user) << "on_list:" << on_list);
	// send subscribe presence to user
	Tag *tag = new Tag("presence");
	tag->addAttribute("type", "subscribe" );
	tag->addAttribute("from", (std::string) remote_user + "@" + p->jid());
	tag->addAttribute("to", m_jid);

	if (alias) {
		Tag *nick = new Tag("nick", (std::string) alias);
		nick->addAttribute("xmlns","http://jabber.org/protocol/nick");
		tag->addChild(nick);
	}

	std::cout << tag->xml() << "\n";
	p->j->send(tag);
}

/*
 * Called when we're ready to connect (we know caps)
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
	if (purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str()) != NULL){
		Log(m_jid, "this account already exists");
		m_account = purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str());
	}
	else {
		Log(m_jid, "creating new account");
		m_account = purple_account_new(m_username.c_str(), this->p->protocol()->protocol().c_str());
		purple_account_set_string(m_account,"encoding","windows-1250");

		purple_accounts_add(m_account);
	}

	m_account->ui_data = this;

	m_loadingBuddiesFromDB = true;
	setRoster(GlooxMessageHandler::instance()->sql()->getBuddies(storageId(), account()));
	m_loadingBuddiesFromDB = false;

	m_connectionStart = time(NULL);
	m_readyForConnect = false;
	if (!m_bindIP.empty())
		purple_account_set_string(m_account,"bind",std::string(m_bindIP).c_str());
	purple_account_set_password(m_account,m_password.c_str());
	Log(m_jid, "UIN:" << m_username << " USER_ID:" << m_userID);

	if (p->configuration().useProxy) {
		PurpleProxyInfo *info = purple_proxy_info_new();
		purple_proxy_info_set_type(info, PURPLE_PROXY_USE_ENVVAR);
		info->username = NULL;
		info->password = NULL;
		purple_account_set_proxy_info(m_account, info);
	}


	// check if it's valid uin
	bool valid = false;
	if (!m_username.empty()) {
		valid = p->protocol()->isValidUsername(m_username);
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

void User::receivedSubscription(const Subscription &subscription) {
	if (m_connected) {
		if (subscription.subtype() == Subscription::Subscribed) {
			// we've already received auth request from legacy server, so we should respond to this request
			if (hasAuthRequest((std::string) subscription.to().username())) {
				Log(m_jid, "subscribed presence for authRequest arrived => accepting the request");
				m_authRequests[(std::string) subscription.to().username()].authorize_cb(m_authRequests[(std::string) subscription.to().username()].user_data);
				m_authRequests.erase(subscription.to().username());
			}
			// subscribed user is not in roster, so we must add him/her there.
			std::string name(subscription.to().username());
			std::for_each( name.begin(), name.end(), replaceJidCharacters() );
			PurpleBuddy *buddy = purple_find_buddy(m_account, name.c_str());
			if (!isInRoster(subscription.to().username(),"both")) {
				if (!buddy) {
					Log(m_jid, "user is not in legacy network contact lists => nothing to be subscribed");
					// this user is not in ICQ contact list... It's nothing to do here,
					// because there is no user to authorize
				} else {
					Log(m_jid, "adding this user to local roster and sending presence");
					if (!isInRoster(subscription.to().username(),"ask")) {
						// add user to mysql database and to local cache
						addRosterItem(buddy);
						p->sql()->addBuddy(m_userID, (std::string) subscription.to().username(), "both");
					}
					else {
						getRosterItem(subscription.to().username())->setSubscription("both");
						p->sql()->updateBuddySubscription(m_userID, (std::string) subscription.to().username(), "both");
					}
					// user is in ICQ contact list so we can inform jabber user
					// about status
					SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
					Tag *tag = s_buddy->generatePresenceStanza(m_features);
					if (tag) {
						tag->addAttribute("to", m_jid);
						p->j->send(tag);
					}
				}
			}
			// it can be reauthorization...
			if (buddy) {
				purpleReauthorizeBuddy(buddy);
			}
			return;
		}
		else if (subscription.subtype() == Subscription::Subscribe) {
			std::string name(subscription.to().username());
			std::for_each( name.begin(), name.end(), replaceJidCharacters() );
			PurpleBuddy *b = purple_find_buddy(m_account, name.c_str());
			if (b) {
				purpleReauthorizeBuddy(b);
			}
			if (isInRoster(subscription.to().username(), "")) {
				SpectrumBuddy *s_buddy = (SpectrumBuddy *) getRosterItem(subscription.to().username());
				if (s_buddy->getSubscription() == "ask") {
					Tag *reply = new Tag("presence");
					reply->addAttribute( "to", subscription.from().bare() );
					reply->addAttribute( "from", subscription.to().bare() );
					reply->addAttribute( "type", "subscribe" );
					p->j->send( reply );
				}
				Log(m_jid, "subscribe presence; user is already in roster => sending subscribed");
				// username is in local roster (he/her is in ICQ roster too),
				// so we should send subscribed presence
				Tag *reply = new Tag("presence");
				reply->addAttribute( "to", subscription.from().bare() );
				reply->addAttribute( "from", subscription.to().bare() );
				reply->addAttribute( "type", "subscribed" );
				p->j->send( reply );
			}
			else {
				Log(m_jid, "subscribe presence; user is not in roster => adding to legacy network");
				// this user is not in local roster, so we have to send add_buddy request
				// to our legacy network and wait for response
				std::string name(subscription.to().username());
				std::for_each( name.begin(), name.end(), replaceJidCharacters() );
				PurpleBuddy *buddy = purple_buddy_new(m_account, name.c_str(), name.c_str());
				purple_blist_add_buddy(buddy, NULL, NULL ,NULL);
				purple_account_add_buddy(m_account, buddy);
			}
			return;
		} else if(subscription.subtype() == Subscription::Unsubscribe || subscription.subtype() == Subscription::Unsubscribed) {
			std::string name(subscription.to().username());
			std::for_each( name.begin(), name.end(), replaceJidCharacters() );
			PurpleBuddy *buddy = purple_find_buddy(m_account, name.c_str());
			if (subscription.subtype() == Subscription::Unsubscribed) {
				// user respond to auth request from legacy network and deny it
				if (hasAuthRequest((std::string) subscription.to().username())) {
					Log(m_jid, "unsubscribed presence for authRequest arrived => rejecting the request");
					m_authRequests[(std::string) subscription.to().username()].deny_cb(m_authRequests[(std::string) subscription.to().username()].user_data);
					m_authRequests.erase(subscription.to().username());
					return;
				}
			}
			if (buddy) {
				Log(m_jid, "unsubscribed presence => removing this contact from legacy network");
				long id = 0;
				if (buddy->node.ui_data) {
					SpectrumBuddy *s_buddy = (SpectrumBuddy *) buddy->node.ui_data;
					id = s_buddy->getId();
				}
				p->sql()->removeBuddy(m_userID, subscription.to().username(), id);
				// thi contact is in ICQ contact list, so we can remove him/her
				purple_account_remove_buddy(m_account, buddy, purple_buddy_get_group(buddy));
				purple_blist_remove_buddy(buddy);
			} else {
				// this contact is not in ICQ contact list, so there is nothing to remove...
			}
			removeFromLocalRoster(subscription.to().username());

			// inform user about removing this contact
			Tag *tag = new Tag("presence");
			tag->addAttribute("to", subscription.from().bare());
			tag->addAttribute("from", subscription.to().username() + "@" + p->jid() + "/bot");
			if(subscription.subtype() == Subscription::Unsubscribe) {
				tag->addAttribute( "type", "unsubscribe" );
			} else {
				tag->addAttribute( "type", "unsubscribed" );
			}
			p->j->send( tag );
			return;
		}
	}
}

/*
 * Received jabber presence...
 */
void User::receivedPresence(const Presence &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag)
		return;
	if (m_lang == NULL) {
		std::string lang = stanzaTag->findAttribute("xml:lang");
		if (lang == "")
			lang = "en";
		setLang(lang.c_str());
		localization.loadLocale(getLang());
		Log("LANG", lang << " " << lang.c_str());
	}

	if (p->protocol()->onPresenceReceived(this, stanza)) {
		delete stanzaTag;
		return;
	}

	// this presence is for the transport
	if (stanza.to().username() == ""  || ((p->protocol()->tempAccountsAllowed()) || p->protocol()->isMUC(NULL, stanza.to().bare()))) {
		if (stanza.presence() == Presence::Unavailable) {
			// disconnect from legacy network if we are connected
			if (stanza.to().username() == "") {
				if ((m_connected == false && int(time(NULL)) > int(m_connectionStart) + 10) || m_connected == true) {
					if (hasResource(stanza.from().resource())) {
						removeResource(stanza.from().resource());
						removeConversationResource(stanza.from().resource());
					}
					sendUnavailablePresenceToAll();
				}
			}
			if (m_connected) {
				if (getResources().empty() || (p->protocol()->tempAccountsAllowed() && !hasOpenedMUC())){
					Log(m_jid, "disconecting");
					purple_account_disconnect(m_account);
					p->userManager()->removeUserTimer(this);
				}
				else {
					setActiveResource();
				}
// 				m_connected=false;
			}
			else {
				if (!getResources().empty() && int(time(NULL)) > int(m_connectionStart) + 10) {
					setActiveResource();
				}
				else if (m_account) {
					Log(m_jid, "disconecting2");
					purple_account_disconnect(m_account);
				}
			}
		} else {
			if (hasResource(stanza.from().resource())) {
				setResource(stanza); // update this resource
				sendPresenceToAll(stanza.from().full());
			}

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
		// send presence about tranport status to user
		if (m_connected || m_readyForConnect) {
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

void User::addFiletransfer( const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size ) {
	FiletransferRepeater *ft = new FiletransferRepeater(p, to, sid, type, from, size);
	g_hash_table_replace(m_filetransfers, g_strdup(to.bare() == m_jid ? from.username().c_str() : to.username().c_str()), ft);
	Log("filetransfer", "adding FT Class as jid:" << std::string(to.bare() == m_jid ? from.username() : to.username()));
}
void User::addFiletransfer( const JID& to ) {
	FiletransferRepeater *ft = new FiletransferRepeater(p, to, m_jid + "/" + getResource().name);
	g_hash_table_replace(m_filetransfers, g_strdup(to.username().c_str()), ft);
}

User::~User(){
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

	m_authRequests.clear();
	g_hash_table_destroy(m_settings);
	g_hash_table_destroy(m_filetransfers);

	p->protocol()->onDestroy(this);
}


