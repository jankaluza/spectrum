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

#ifndef _HI_ABSTRACTPROTOCOL_H
#define _HI_ABSTRACTPROTOCOL_H

#include <cctype>
#include <string>
#include <algorithm>
#include <list>
#include "purple.h"
#include "glib.h"
#include "gloox/tag.h"
#include "gloox/message.h"
#include "../abstractuser.h"
#include "../spectrum_util.h"
#include "../log.h"
#include "../localization.h"
#include "../protocolmanager.h"

extern Localization localization;

using namespace gloox;

// Abstract class for wrapping libpurple protocols
class AbstractProtocol
{
	public:
		virtual ~AbstractProtocol() {}

		// Returns gateway identity (http://xmpp.org/registrar/disco-categories.html).
		virtual const std::string gatewayIdentity() = 0;

		// Returns purple protocol id (for example "prpl-icq" for ICQ protocol).
		// This id has to be obtained from prpl source code from PurplePluginProtocolInfo structure.
		virtual const std::string protocol() = 0;

		// Returns true if username is valid username for this protocol.
		virtual bool isValidUsername(const std::string &username) { return true; };

		// Returns revised username (for example for ICQ where username = "123- 456-789"; return "123456789";)
		virtual void prepareUsername(std::string &username, PurpleAccount *account = NULL) {
			username.assign(purple_normalize(account, username.c_str()));
			std::transform(username.begin(), username.end(), username.begin(), (int(*)(int)) std::tolower);
		};

		// This function should create bare JID from any purpleConversationName. The name is obtained
		// by purple_conversation_get_name(conv) function where conv is PurpleConversation of 
		// PURPLE_CONV_TYPE_CHAT type.
		// Examples:
		// For IRC: #pidgin" -> "#pidgin%irc.freenode.net@irc.spectrum.im"
		// For XMPP: spectrum@conference.spectrum.im/something" -> "spectrum%conference.spectrum.im@irc.spectrum.im"
		// Note: You can ignore this function if your prpl doesn't support MUC (Groupchat).
		virtual void makeRoomJID(AbstractUser *user, std::string &purpleConversationName) { }

		// This function should create purple username from JID defined in 'to' attribute in incoming message.
		// Purple username created by this function is then used to create new PurpleConversation. This function
		// is called for PMs (Private Messages) in MUC.
		// Examples:
		// For IRC: "#pidgin%irc.freenode.org@irc.spectrum.im/HanzZ" -> "HanzZ"
		// For IRC "#pidgin%irc.freenode.org@irc.spectrum.im" -> #pidgin
		// For XMPP: "spectrum%conference.spectrum.im@irc.spectrum.im/HanzZ" -> "spectrum@conference.spectrum.im/HanzZ"
		// For XMPP: "spectrum%conference.spectrum.im@irc.spectrum.im" -> "spectrum@conference.spectrum.im"
		// Note: You can ignore this function if your prpl doesn't support MUC (Groupchat).
		virtual void makePurpleUsernameRoom(AbstractUser *user, const JID &jid, std::string &name) { }

		// This function should create username of room ocupant from libpurple's "who" argument
		// of purple_conversation_im_write function. New username is then used as resource for JID created by makeRoomJID.
		// Examples:
		// For XMPP: "spectrum@conference.spectrum.im/HanzZ" -> "HanzZ"
		// Note: You can ignore this function if your prpl doesn't support MUC (Groupchat).
		virtual void makeUsernameRoom(AbstractUser *user, std::string &name) {
			name = JID(name).resource();
		}

		// Tries to find PurpleChat. This function is called before joining the room to check if the room is not stored
		// in buddy list.
		virtual PurpleChat *getPurpleChat(AbstractUser *user, const std::string &purpleUsername) {
			return purple_blist_find_chat(user->account(), purpleUsername.c_str());
		}

		// This function should create purple username from JID defined in 'to' attribute in incoming message.
		// Purple username created by this function is then used to create new PurpleConversation. This function
		// is called for type="chat" messages. By default, it tries to JID:escapeNode() and changes % to @. For normal
		// prpls you don't have to change it.
		// Examples:
		// Default: "hanzz%test.im@xmpp.spectrum.im/bot" -> "hanzz@test.im"
		// Default: "hanzz\40test.im@xmpp.spectrum.im/bot" -> "hanzz@test.im"
		// For IRC: "hanzz%irc.freenode.org@irc.spectrum.im/bot" -> "hanzz"
		virtual void makePurpleUsernameIM(AbstractUser *user, const JID &jid, std::string &name) {
			name.assign(purpleUsername(jid.username()));
		}

		// Returns disco features used by transport jid.
		virtual std::list<std::string> transportFeatures() = 0;

		// Returns disco features used by transport jid.
		virtual std::list<std::string> buddyFeatures() = 0;
		
		// Returns pregenerated text according to key.
		virtual std::string text(const std::string &key) = 0;

		// Parses VCard and returns VCard Tag*.
		virtual Tag *getVCardTag(AbstractUser *user, GList *vcardEntries) { return NULL; }

		virtual bool getXStatus(const std::string &mood, std::string &type, std::string &xmlns, std::string &tag1, std::string &tag2) { return false; }

		/*
		 * Returns true if this jid is jid of MUC
		 */
		virtual bool isMUC(AbstractUser *user, const std::string &jid) { return false; }
		/*
		 * Returns the username of contact from which notifications will be sent
		 */
		virtual const std::string notifyUsername() { return gloox::EmptyString; }
		/*
		 * Returns the name of protocol actions for user search or empty string if there is not any
		 */
		virtual const std::string userSearchAction() { return gloox::EmptyString; }
		/*
		 * Returns ID of column used for UIN/name/ID of founded users in searchresults
		 */
		virtual const std::string userSearchColumn() { return gloox::EmptyString; }
		/*
		 * Returns true if temporary accounts for MUC are allows (this is useful for IRC, if you want to connect more network from one account)
		 */
		virtual bool tempAccountsAllowed() { return false; }
		/*
		 * Change nickname in room. Returns true if the nickname has to be changed in all rooms user is in.
		 */
		virtual bool changeNickname(const std::string &nick, PurpleConversation *conv) { return true; }
		
		virtual std::string defaultEncoding() { return "utf8"; }

		// SIGNALS

		/*
		 * Called when new user is created
		 */
		virtual void onUserCreated(AbstractUser *user) { }
		/*
		 * Called when user gets connected
		 */
		virtual void onConnected(AbstractUser *user) { }
		/*
		 * Called when user class is getting destroyed
		 */
		virtual void onDestroy(AbstractUser *user) { }
		/*
		 * Presence Received. Returns true if the presence was handled.
		 */
		virtual bool onPresenceReceived(AbstractUser *user, const Presence &stanza) { return false; }
		/*
		 * Called on purple_request_input.
		 */
		virtual bool onPurpleRequestInput(void *handle, AbstractUser *user, const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) { return false; }
		virtual void onPurpleAccountCreated(PurpleAccount *account) {}
		virtual bool onNotifyUri(const char *uri) { return false; }
		virtual void onRequestClose(void *handle) { }
		virtual void onXMPPMessageReceived(AbstractUser *user, const Message &msg) {}
};

#endif
