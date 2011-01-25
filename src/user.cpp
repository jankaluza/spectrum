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

#ifdef WITH_IMAGEMAGICK
#include "Magick++.h"
#endif

User::User(JID jid, const std::string &username, const std::string &password, const std::string &userKey, long id, const std::string &encoding, const std::string &language, bool vip) : SpectrumRosterManager(this), SpectrumMessageHandler(this), SettingsManager(this) {
	m_jid = jid.bare();

	setResource(jid.resource());
	setActiveResource(jid.resource());

	m_userID = id;
	m_userKey = userKey;
	m_account = NULL;
	m_vip = vip;
	m_readyForConnect = false;
	m_connected = false;
	m_reconnectCount = 0;
	m_glooxPresenceType = -1;

	m_encoding = encoding;
	m_lang = g_strdup(language.c_str());
	m_features = 0;
	m_loadingBuddiesFromDB = false;
	m_photoHash.clear();

	m_password = password;
	m_username = username;
	if (!CONFIG().username_mask.empty()) {
		std::string newUsername(CONFIG().username_mask);
		replace(newUsername, "$username", m_username.c_str());
		m_username = newUsername;
	}

	setSettings(Transport::instance()->sql()->getSettings(m_userID));

	// Add default settings
	addSetting("enable_transport", true);
	addSetting("enable_notify_email", false);
	addSetting("enable_avatars", true);
	addSetting("enable_chatstate", true);
	addSetting("save_files_on_server", false);
	addSetting("reject_authorizations", false);
	addSetting("first_synchronization_done", false);

	Transport::instance()->sql()->setUserOnline(m_userID, true);
	Transport::instance()->protocol()->onUserCreated(this);

	// Ask for user's roster
	sendRosterGet(m_jid);
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
 * Called when legacy network user stops typing.
 */
void User::purpleBuddyTypingStopped(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	if (!getSetting<bool>("enable_chatstate"))
		return;

	Log(m_jid, uin << " stopped typing");

	std::string username(uin);
	// Remove resource if it's XMPP JID
	if (Transport::instance()->getConfiguration().protocol == "xmpp") {
		size_t pos = username.find("/");
		if (pos != std::string::npos)
			username.erase((int) pos, username.length() - (int) pos);
	}
	AbstractSpectrumBuddy *s_buddy = getRosterItem(uin);
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		username = JID::escapeNode(username);
	else
		std::for_each( username.begin(), username.end(), replaceBadJidCharacters() ); // OK

	SpectrumMessageHandler::sendChatstate(username + "@" + Transport::instance()->jid() + "/bot", m_jid, "active");
}

/*
 * Called when legacy network user starts typing.
 */
void User::purpleBuddyTyping(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	if (!getSetting<bool>("enable_chatstate"))
		return;

	Log(m_jid, uin << " is typing");

	std::string username(uin);
	// Remove resource if it's XMPP JID
	if (Transport::instance()->getConfiguration().protocol == "xmpp") {
		size_t pos = username.find("/");
		if (pos != std::string::npos)
			username.erase((int) pos, username.length() - (int) pos);
	}

	AbstractSpectrumBuddy *s_buddy = getRosterItem(uin);
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		username = JID::escapeNode(username);
	else
		std::for_each( username.begin(), username.end(), replaceBadJidCharacters() ); // OK

	SpectrumMessageHandler::sendChatstate(username + "@" + Transport::instance()->jid() + "/bot", m_jid, "composing");
}

/*
 * Called when legacy network user stops typing.
 */
void User::purpleBuddyTypingPaused(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES) || !hasTransportFeature(TRANSPORT_FEATURE_TYPING_NOTIFY))
		return;
	if (!getSetting<bool>("enable_chatstate"))
		return;

	Log(m_jid, uin << " paused typing");

	std::string username(uin);
	// Remove resource if it's XMPP JID
	if (Transport::instance()->getConfiguration().protocol == "xmpp") {
		size_t pos = username.find("/");
		if (pos != std::string::npos)
			username.erase((int) pos, username.length() - (int) pos);
	}

	AbstractSpectrumBuddy *s_buddy = getRosterItem(uin);
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		username = JID::escapeNode(username);
	else
		std::for_each( username.begin(), username.end(), replaceBadJidCharacters() ); // OK

	SpectrumMessageHandler::sendChatstate(username + "@" + Transport::instance()->jid() + "/bot", m_jid, "paused");
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

	if (purple_accounts_find(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str()) != NULL){
		Log(m_jid, "this account already exists");
		m_account = purple_accounts_find(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str());
		User *user = (User *) Transport::instance()->userManager()->getUserByAccount(m_account);
		if (user && user != this) {
			m_account = NULL;
			Log(m_jid, "This account is already connected by another jid " << user->jid());
			return;
		}
	}
	else {
		Log(m_jid, "creating new account");
		m_account = purple_account_new(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str());

		purple_accounts_add(m_account);
	}
	Transport::instance()->collector()->stopCollecting(m_account);

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
			case PURPLE_PREF_STRING_LIST:
				if (v.str)
					purple_account_set_string(m_account, key.c_str(), v.str);
				else
					purple_account_remove_setting(m_account, key.c_str());
				break;
			default:
				continue;
		}
	}

	purple_account_set_string(m_account, "encoding", m_encoding.empty() ? Transport::instance()->getConfiguration().encoding.c_str() : m_encoding.c_str());
// 	purple_account_set_bool(m_account, "use_clientlogin", false);
	purple_account_set_bool(m_account, "require_tls",  Transport::instance()->getConfiguration().require_tls);
	purple_account_set_bool(m_account, "use_ssl",  Transport::instance()->getConfiguration().require_tls);
	purple_account_set_bool(m_account, "direct_connect", false);
	purple_account_set_bool(m_account, "check-mail", getSetting<bool>("enable_notify_email"));

	m_account->ui_data = this;
	
	Transport::instance()->protocol()->onPurpleAccountCreated(m_account);

	m_loadingBuddiesFromDB = true;
	loadRoster();
	m_loadingBuddiesFromDB = false;

	m_readyForConnect = false;
	purple_account_set_password(m_account,m_password.c_str());
	Log(m_jid, "UIN:" << m_username << " USER_ID:" << m_userID);

	if (CONFIG().useProxy) {
		PurpleProxyInfo *info = purple_proxy_info_new();
		purple_proxy_info_set_type(info, PURPLE_PROXY_USE_ENVVAR);
		info->username = NULL;
		info->password = NULL;
		purple_account_set_proxy_info(m_account, info);
	}

	if (valid && getSetting<bool>("enable_transport")) {
		purple_account_set_enabled(m_account, PURPLE_UI, TRUE);
// 		purple_account_connect(m_account);
		const PurpleStatusType *statusType = purple_account_get_status_type_with_primitive(m_account, (PurpleStatusPrimitive) m_presenceType);
		if (statusType) {
			Log(m_jid, "Setting up default status.");
			if (!m_statusMessage.empty()) {
				purple_account_set_status(m_account, purple_status_type_get_id(statusType), TRUE, "message", m_statusMessage.c_str(), NULL);
			}
			else {
				purple_account_set_status(m_account, purple_status_type_get_id(statusType), TRUE, NULL);
			}
		}
	}

	SpectrumRosterManager::sendPresence(Transport::instance()->jid(), m_jid, "unavailable", tr(getLang(), _("Connecting")));
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
		std::string name = JID(stanza->findAttribute("to")).resource();

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
			// for XMPP we have to store bare jid there...
			std::string bare = JID(name).bare();
			if (!bare.empty())
				setRoomResource(bare, JID(stanza->findAttribute("from")).resource());
			serv_join_chat(gc, comps);
			g_hash_table_destroy(comps);
		}
		delete (*it);
	};

	m_autoConnectRooms.clear();

	if (m_glooxPresenceType != -1) {
		SpectrumRosterManager::sendPresence(Transport::instance()->jid(), m_jid,
											(Presence::PresenceType) m_glooxPresenceType, m_statusMessage);
		m_glooxPresenceType = -1;
	}
	mergeRoster();
}

/*
 * Received jabber presence...
 */
void User::receivedPresence(const Presence &stanza) {
	Tag *stanzaTag = stanza.tag();
	if (!stanzaTag)
		return;
	bool statusChanged = false;

// 	std::string lang = stanzaTag->findAttribute("xml:lang");
// 	if (lang != "") {
// 		setLang(lang.c_str());
// 		localization.loadLocale(getLang());
// 	}
	
	if (stanza.to().username() == "" && isConnected() && stanza.presence() != Presence::Unavailable) {
		Tag *x_vcard = stanzaTag->findChild("x");
		if (x_vcard) {
			Tag *photo = x_vcard->findChild("photo");
			if (photo && !photo->cdata().empty()) {
				if (photo->cdata() != m_photoHash) {
					Transport::instance()->fetchVCard(jid());
				}
			}
		}
	}
	
	handlePresence(stanza);


	// Handle join-the-room presence
	bool isMUC = stanza.findExtension(ExtMUC) != NULL;
	if (isMUC && stanza.to().username() != "" && !isOpenedConversation(stanza.to().bare())) {
		if (stanza.presence() != Presence::Unavailable) {
			if (m_connected) {
				GHashTable *comps = NULL;
				std::string name = stanza.to().resource();

				PurpleConnection *gc = purple_account_get_connection(m_account);
				Transport::instance()->protocol()->makePurpleUsernameRoom(this, stanza.to().bare(), name);
				PurpleChat *chat = Transport::instance()->protocol()->getPurpleChat(this, name);
				if (chat) {
					comps = purple_chat_get_components(chat);
				}
				else if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL) {
					comps = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, name.c_str());
				}
				if (comps) {
					std::cout << name << " JOINING\n";
					setRoomResource(name, stanza.from().resource());
					std::string bare = JID(name).bare();
					if (!bare.empty())
						setRoomResource(bare, stanza.from().resource());
					serv_join_chat(gc, comps);
				}
			}
			else {
				m_autoConnectRooms.push_back(stanza.tag());
			}
		}
	}

	std::cout << "Unavailable" << stanza.to().username() << "\n";
	if (stanza.to().username() != ""  && stanza.presence() == Presence::Unavailable) {
		std::string k = stanza.to().bare();
		removeConversation(k, true);
	}

	// this presence is for the transport
	if (stanza.to().username() == ""  || (Transport::instance()->protocol()->tempAccountsAllowed())) {
		if (stanza.presence() == Presence::Unavailable) {
			// disconnect from legacy network if we are connected
			if (stanza.to().username() == "") {
// 				if ((m_connected == false && int(time(NULL)) > int(m_connectionStart) + 10) || m_connected == true) {
					if (hasResource(stanza.from().resource())) {
						removeResource(stanza.from().resource());
						removeConversationResource(stanza.from().resource());
						GlooxAdhocHandler::instance()->unregisterSession(stanza.from().full());
					}
					sendUnavailablePresenceToAll(stanza.from().resource());
// 				}
			}
			if (m_connected) {
				if (getResources().empty() || (Transport::instance()->protocol()->tempAccountsAllowed() && !hasOpenedMUC())){
					Log(m_jid, "disconecting");
// 					sendUnavailablePresenceToAll();
// 					purple_account_disconnect(m_account);
					GlooxAdhocHandler::instance()->unregisterSession(stanza.from().full());
				}
				else {
					setActiveResource();
					// Active resource changed, so we probably want to update status message/show.
					forwardStatus(getResource().show, getResource().status);
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
// 					purple_account_disconnect(m_account);
				}
			}
		} else {
			std::string oldActiveResource = getResource().name;
			if (!hasResource(stanza.from().resource())) {
				setResource(stanza); // add this resource
				sendPresenceToAll(stanza.from().full());
			}
			else
				setResource(stanza); // update this resource

			int presence = stanza.presence();
			std::string stanzaStatus = stanza.status();
				
			// Active resource has been changed, so we have to inform SpectrumMessageHandler to send
			// next message from legacy network to bare jid.
			if (getResource().name != oldActiveResource) {
				removeConversationResource(oldActiveResource);
				presence = getResource().show;
				stanzaStatus = getResource().status;
			}

			Log(m_jid, "RESOURCE" << getResource().name << " " << stanza.from().resource());
			if (!m_connected) {
				statusChanged = true;
				// we are not connected to legacy network, so we should do it when disco#info arrive :)
				Log(m_jid, "connecting: resource=" << getResource().name);
				if (m_readyForConnect == false) {
					m_readyForConnect = true;
					// Forward status message to legacy network, but only if it's sent from active resource
					if (getResource().name == stanza.from().resource()) {
						forwardStatus(presence, stanzaStatus);
					}
					if (getResource().caps == -1) {
						// caps not arrived yet, so we can't connect just now and we have to wait for caps
					}
					else {
						connect();
					}
				}
			}
			// Forward status message to legacy network, but only if it's sent from active resource
			else if (getResource().name == stanza.from().resource()) {
				Log(m_jid, "mirroring presence to legacy network");
				statusChanged = true;
				forwardStatus(presence, stanzaStatus);
			}
			// Just forward user presence to XMPP user, not change it in legacy network
			else {
				SpectrumRosterManager::sendPresence(Transport::instance()->jid(), m_jid,
													(Presence::PresenceType) presence, stanzaStatus);
			}
		}

		if (getSetting<bool>("enable_transport") == false) {
			SpectrumRosterManager::sendPresence(Transport::instance()->jid(), stanza.from().bare(),
												"unavailable");
		}
		// send presence about tranport status to user
		else if (getResources().empty()) {
			SpectrumRosterManager::sendPresence(Transport::instance()->jid(), stanza.from().bare(),
												"unavailable");
		}
		else if (stanza.presence() == Presence::Unavailable) {
			SpectrumRosterManager::sendPresence(Transport::instance()->jid(), stanza.from().full(),
												stanza.presence(), stanza.status());
		}
// 		else if (statusChanged) {
// 			Presence tag(stanza.presence(), m_jid, stanza.status());
// 			tag.setFrom(Transport::instance()->jid());
// 			Transport::instance()->send(tag);
// 		}

	}
	delete stanzaTag;
}

void User::forwardStatus(int presence, const std::string &stanzaStatus) {
	int PurplePresenceType;
	// mirror presence types
	switch (presence) {
		case Presence::Available: {
			PurplePresenceType = PURPLE_STATUS_AVAILABLE;
			break;
		}
		case Presence::Chat: {
			PurplePresenceType = PURPLE_STATUS_AVAILABLE;
			break;
		}
		case Presence::Away: {
			if (purple_account_get_status_type_with_primitive(m_account, PURPLE_STATUS_AWAY))
				PurplePresenceType = PURPLE_STATUS_AWAY;
			else
				PurplePresenceType = PURPLE_STATUS_EXTENDED_AWAY;
			break;
		}
		case Presence::DND: {
			PurplePresenceType = PURPLE_STATUS_UNAVAILABLE;
			break;
		}
		case Presence::XA: {
			if (purple_account_get_status_type_with_primitive(m_account, PURPLE_STATUS_EXTENDED_AWAY))
				PurplePresenceType = PURPLE_STATUS_EXTENDED_AWAY;
			else
				PurplePresenceType = PURPLE_STATUS_AWAY;
			break;
		}
		default:
			PurplePresenceType = PURPLE_STATUS_AVAILABLE;
			break;
	}
	
	gchar *_markup = purple_markup_escape_text(stanzaStatus.c_str(), -1);
	std::string markup(_markup);
	g_free(_markup);

	if (!m_connected) {
		m_presenceType = PurplePresenceType;
		m_statusMessage = markup;
		m_glooxPresenceType = presence;
	}
	else {
		// we are already connected so we have to change status
		const PurpleStatusType *status_type = purple_account_get_status_type_with_primitive(m_account, (PurpleStatusPrimitive) PurplePresenceType);
		if (status_type != NULL) {
			// send presence to legacy network
			if (!markup.empty()) {
				purple_account_set_status(m_account, purple_status_type_get_id(status_type), TRUE, "message", markup.c_str(), NULL);
			}
			else {
				purple_account_set_status(m_account, purple_status_type_get_id(status_type), TRUE, NULL);
			}
		}

		SpectrumRosterManager::sendPresence(Transport::instance()->jid(), m_jid,
											(Presence::PresenceType) presence, statusMessage);
	}
}

void User::handleVCard(const VCard* vcard) {
	Log("handleVCard", "setting account icon for " << m_jid);
	gssize size = vcard->photo().binval.size();
	// this will be freed by libpurple
	guchar *photo = (guchar *) g_malloc(size * sizeof(guchar));
	memcpy(photo, vcard->photo().binval.c_str(), size);

#ifdef WITH_IMAGEMAGICK
	if (size != 0) {
		PurplePlugin *plugin = purple_find_prpl(Transport::instance()->protocol()->protocol().c_str());
		PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(plugin);
		if (prpl_info->icon_spec.format == NULL) {
			g_free(photo);
			return;
		}
		try {
			Magick::Blob blob(photo, size);
			g_free(photo);
			photo = NULL;
			Magick::Image img(blob);

			std::string format;
			gchar **strlist = g_strsplit(prpl_info->icon_spec.format, ",", 10);
			for (gchar **f = strlist; *f != NULL; f++) {
				// jpeg is the best
				if (strcmp(*f, "jpeg") == 0 || strcmp(*f, "jpg") == 0) {
					format = "jpg";
				}
				// png is number two
				else if (strcmp(*f, "png") == 0 && format != "jpg") {
					format = *f;
				}
				// gif is alright if there's not jpeg or png
				else if (strcmp(*f, "gif") == 0 && format != "jpg" && format != "png") {
					format = *f;
				}
				else if (format.empty()) {
					format = *f;
				}
			}
			g_strfreev(strlist);
			img.magick(format);

			int width, height;
			if (CONFIG().protocol == "icq") {
				width = 48;
				height = 48;
			}
			else {
				purple_buddy_icon_get_scale_size(&prpl_info->icon_spec, &width, &height);
			}

			if (img.size().width() != width || img.size().height() != height) {
				Magick::Geometry g = Magick::Geometry(width,height);
				g.aspect(CONFIG().protocol == "icq");
				img.scale(g);
			}

			Magick::Blob output;
			img.write(&output);
			size = output.length();
			// this will be freed by libpurple
			photo = (guchar *) g_malloc(size * sizeof(guchar));
			memcpy(photo, output.data(), size);
		}
		catch ( Magick::Exception & error) {
			Log("handleVCard","Caught Magick++ exception: " << error.what());
		} catch(...) {   // catch all other exceptions
		}
	}
#endif /* WITH_IMAGEMAGICK */

	if (!photo)
		return;
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

	// purple_account_destroy(m_account);
	// delete(m_account);
	// if (this->save_timer!=0 && this->save_timer!=-1)
	// 	std::cout << "* removing timer\n";
	// 	purple_timeout_remove(this->save_timer);

	if (m_account) {
		GList *iter;
		// Remove filetransfers
		for (iter = purple_xfers_get_all(); iter; ) {
			PurpleXfer *xfer = (PurpleXfer *) iter->data;
			iter = iter->next;
			if (purple_xfer_get_account(xfer) == m_account) {
				if (purple_xfer_get_status(xfer) != PURPLE_XFER_STATUS_DONE) {
					purple_xfer_cancel_local(xfer);
				}
			}
		}

		// Remove conversations.
		// This has to be called before m_account->ui_data = NULL;, because it uses
		// ui_data to call SpectrumMessageHandler::purpleConversationDestroyed() callback.
		for (iter = purple_get_conversations(); iter; ) {
			PurpleConversation *conv = (PurpleConversation*) iter->data;
			iter = iter->next;
			if (purple_conversation_get_account(conv) == m_account)
				purple_conversation_destroy(conv);
		}

		purple_account_set_enabled(m_account, PURPLE_UI, FALSE);
		m_account->ui_data = NULL;
		Transport::instance()->collector()->collect(m_account);
	}
// 	else {
// 		PurpleAccount *act = purple_accounts_find(m_username.c_str(), Transport::instance()->protocol()->protocol().c_str());
// 		if (act) {
// 			act->ui_data = NULL;
// 			Transport::instance()->collector()->collect(act);
// 			purple_account_set_enabled(act, PURPLE_UI, FALSE);
// 		}
// 	}
}


