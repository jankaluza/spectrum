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

/*
 * Called when contact list has been received from legacy network.
 */
static gboolean sync_cb(gpointer data)
{
	User *d;
	d = (User*) data;
	return d->syncCallback();
}

User::User(GlooxMessageHandler *parent, const std::string &jid, const std::string &username, const std::string &password){
	p = parent;
	m_jid = jid;
	m_username = username;
	m_password = password;
	m_connected = false;
	m_roster = p->sql()->getRosterByJid(m_jid);
	m_vip = p->sql()->isVIP(m_jid);
	m_syncTimer = 0;
	m_readyForConnect = false;
	m_rosterXCalled = false;
	m_account = NULL;
	m_connectionStart = time(NULL);
	m_subscribeLastCount = -1;
	m_reconnectCount = 0;
	this->features = 6; // TODO: I can't be hardcoded
}

bool User::syncCallback() {
	Log().Get(jid()) << "sync_cb lastCount: " << m_subscribeLastCount << "cacheSize: " << int(m_subscribeCache.size());
	// we have to check, if all users arrived and repeat this call if not
	if (m_subscribeLastCount == int(m_subscribeCache.size())) {
		syncContacts();
		sendRosterX();
		return FALSE;
	}
	else{
		m_subscribeLastCount = int(m_subscribeCache.size());
		return TRUE;
	}
}

/*
 * Returns true if main JID for this User has feature `feature`,
 * otherwise returns false.
 */
bool User::hasFeature(int feature){
	if (m_capsVersion.empty())
		return false;
	if (p->capsCache[m_capsVersion]&feature)
		return true;
	return false;
}

/*
 * Returns true if transport has feature `feature` for this user,
 * otherwise returns false.
 */
bool User::hasTransportFeature(int feature){
	if (this->features&feature)
		return true;
	return false;
}

/*
 * Returns true if user with UIN `name` and subscription `subscription` is
 * in roster. If subscription.empty(), returns true on any subsciption.
 */
bool User::isInRoster(const std::string &name,const std::string &subscription){
	std::map<std::string,RosterRow>::iterator iter = m_roster.begin();
	iter = m_roster.find(name);
	if(iter != m_roster.end()){
		if (subscription.empty())
			return true;
		if (m_roster[name].subscription==subscription)
			return true;
	}
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

/*
 * Returns true if exists opened conversation with user with uin `name`.
 */
bool User::isOpenedConversation(const std::string &name){
	std::map<std::string,PurpleConversation *>::iterator iter = m_conversations.begin();
	iter = m_conversations.find(name);
	if(iter != m_conversations.end())
		return true;
	return false;
}

/*
 * Sends all legacy network users which are recorded in subscribeCache
 * to this jabber user by roster item exchange.
 */
void User::sendRosterX()
{
	m_rosterXCalled = true;
	m_syncTimer = 0;
	if (int(m_subscribeCache.size())==0)
		return;
	Log().Get(m_jid) << "Sending rosterX";
	Tag *tag = new Tag("iq");
	tag->addAttribute( "to", m_jid +"/"+m_resource);
	tag->addAttribute( "type", "set");
	tag->addAttribute( "id",p->j->getID());
	std::string from;
	from.append(p->jid());
	tag->addAttribute( "from", from );
	Tag *x = new Tag("x");
	x->addAttribute("xmlns","http://jabber.org/protocol/rosterx");
	Tag *item;

	// adding these users to roster db with subscription=ask
	std::map<std::string,subscribeContact>::iterator it = m_subscribeCache.begin();
	while(it != m_subscribeCache.end()) {
		if (!(*it).second.uin.empty()){
			RosterRow user;
			user.id = -1;
			user.jid = m_jid;
			user.uin = (*it).second.uin;
			user.subscription = "ask";
			user.online = false;
			user.lastPresence = "";
			m_roster[(*it).second.uin] = user;
			p->sql()->addUserToRoster(m_jid,(*it).second.uin,"ask");

			item = new Tag("item");
			item->addAttribute("action","add");
			item->addAttribute("jid",(*it).second.uin+"@"+p->jid());
			item->addAttribute("name",(*it).second.alias);
			item->addChild(new Tag("group",(*it).second.group));
			x->addChild(item);
		}
		it++;
	}
	tag->addChild(x);
	std::cout << tag->xml() << "\n";
	p->j->send(tag);
	
	m_subscribeCache.clear();
	m_subscribeLastCount = -1;
}

/*
 * Adds contacts which are not in legacy network, but which are in local jabber roster,
 * to the legacy network.
 */
void User::syncContacts()
{
	PurpleBuddy *buddy;
	Log().Get(m_jid) << "Syncing contacts with legacy network.";
	for(std::map<std::string, RosterRow>::iterator u = m_roster.begin(); u != m_roster.end() ; u++){
		buddy = purple_find_buddy(m_account, (*u).second.uin.c_str());
		// buddy is not in blist, so it's not on server
		if (!buddy) {
			// add buddy to server
			buddy = purple_buddy_new(m_account,(*u).second.uin.c_str(),(*u).second.uin.c_str());
			purple_blist_add_buddy(buddy, NULL, NULL ,NULL);
			purple_account_add_buddy(m_account, buddy);
		}
	}
}

/*
 * Generates presence stanza for PurpleBuddy `buddy`. This will
 * create whole <presence> stanza without 'to' attribute.
 */
Tag *User::generatePresenceStanza(PurpleBuddy *buddy){
	if (buddy==NULL)
		return NULL;
	std::string alias(purple_buddy_get_alias(buddy));
	std::string name(purple_buddy_get_name(buddy));
	PurplePresence *pres = purple_buddy_get_presence(buddy);
	if (pres==NULL)
		return NULL;
	PurpleStatus *stat = purple_presence_get_active_status(pres);
	if (stat==NULL)
		return NULL;
	int s = purple_status_type_get_primitive(purple_status_get_type(stat));
	char *text = NULL;
	char *statusMessage = NULL;

	PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_account_get_connection(m_account)->prpl);

	if (prpl_info && prpl_info->status_text) {
		statusMessage = prpl_info->status_text(buddy);
	}

	Log().Get(m_jid) << "Generating presence stanza for user " << name;
	Tag *tag = new Tag("presence");
	std::string from;
	from.append(name);
	from.append("@");
	from.append(p->jid()+"/bot");
	tag->addAttribute( "from", from );

	if (statusMessage!=NULL){
		std::string _status(statusMessage);
		Log().Get(m_jid) << "Raw status message: " << _status;
		tag->addChild( new Tag("status", stripHTMLTags(_status)));
		g_free(statusMessage);
	}
	
	switch(s) {
		case PURPLE_STATUS_AVAILABLE: {
			break;
		}
		case PURPLE_STATUS_AWAY: {
			tag->addChild( new Tag("show", "away" ) );
			break;
		}
		case PURPLE_STATUS_UNAVAILABLE: {
			tag->addChild( new Tag("show", "dnd" ) );
			break;
		}
		case PURPLE_STATUS_EXTENDED_AWAY: {
			tag->addChild( new Tag("show", "xa" ) );
			break;
		}
		case PURPLE_STATUS_OFFLINE: {
			tag->addAttribute( "type", "unavailable" );
			break;
		}
	}
	
	// caps
	Tag *c = new Tag("c");
	c->addAttribute("xmlns","http://jabber.org/protocol/caps");
	c->addAttribute("hash","sha-1");
	c->addAttribute("node","http://jabbim.cz/icqtransport");
	c->addAttribute("ver","Q543534fdsfsdsssT/WM94uAlu0=");
	tag->addChild(c);
	
	// vcard-temp:x:update
	char *avatarHash = NULL;
	PurpleBuddyIcon *icon = purple_buddy_get_icon(buddy);
	if (icon != NULL) {
		avatarHash = purple_buddy_icon_get_full_path(icon);
	}

	Tag *x = new Tag("x");
	x->addAttribute("xmlns","vcard-temp:x:update");
	if (avatarHash != NULL) {
		Log().Get(m_jid) << "Got avatar hash";
		// Check if it's patched libpurple which saved icons to directories
		char *hash = rindex(avatarHash,'/');
		std::string h;
		if (hash)
			x->addChild(new Tag("photo",(std::string) hash));
		else
			x->addChild(new Tag("photo",(std::string) avatarHash));
	}
	else{
		Log().Get(m_jid) << "no avatar hash";
		x->addChild(new Tag("photo"));
	}
	tag->addChild(x);

	// update stats...
	if (s==PURPLE_STATUS_OFFLINE){
		m_roster[name].online=false;
	}
	else
		m_roster[name].online=true;

	return tag;
}

/*
 * Re-requests authorization for buddy if we can do it (if buddy is not authorized).
 */
void User::purpleReauthorizeBuddy(PurpleBuddy *buddy){
	if (!m_connected)
		return;
	if (!buddy)
		return;
	if (!m_account)
		return;
	GList *l, *ll;
	std::string name(purple_buddy_get_name(buddy));
	if (purple_account_get_connection(m_account)){
		PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_account_get_connection(m_account)->prpl);
		if(prpl_info && prpl_info->blist_node_menu){
			for(l = ll = prpl_info->blist_node_menu((PurpleBlistNode*)buddy); l; l = l->next) {
				if (l->data) {
					PurpleMenuAction *act = (PurpleMenuAction *) l->data;
					if (act->label) {
						Log().Get(m_jid) << (std::string)act->label;
						if ((std::string)act->label == "Re-request Authorization"){
							Log().Get(m_jid) << "rerequesting authorization for " << name;
							((GSourceFunc) act->callback)(act->data);
							break;
						}
					}
				}
			}
			g_list_free(ll);
		}
	}
}

/*
 * Called when something related to this buddy changed...
 */
void User::purpleBuddyChanged(PurpleBuddy *buddy){
	if (buddy==NULL)
		return;
	std::string alias(purple_buddy_get_alias(buddy));
	std::string name(purple_buddy_get_name(buddy));
	PurplePresence *pres = purple_buddy_get_presence(buddy);
	if (pres==NULL)
		return;
	PurpleStatus *stat = purple_presence_get_active_status(pres);
	if (stat==NULL)
		return;
	int s = purple_status_type_get_primitive(purple_status_get_type(stat));
	// TODO: rewrite me to use prpl_info->status_text(buddy)
	const char *statusMessage = purple_status_get_attr_string(stat, "message");

	Log().Get(m_jid) << "purpleBuddyChanged: " << name << " ("<< alias <<") (" << s << ")";

	if (m_syncTimer==0 && !m_rosterXCalled){
		m_syncTimer = purple_timeout_add_seconds(4, sync_cb, this);
	}

	bool inRoster = purple_blist_node_get_bool(&buddy->node, "inRoster");
	if (!inRoster) {
		inRoster = isInRoster(name,"");
		purple_blist_node_set_bool(&buddy->node, "inRoster", true);
	}

	if (!inRoster) {
		if (!m_rosterXCalled && hasFeature(GLOOX_FEATURE_ROSTERX)){
			subscribeContact c;
			c.uin = name;
			c.alias = alias;
			c.group = (std::string) purple_group_get_name(purple_buddy_get_group(buddy));
			m_subscribeCache[name] = c;
			Log().Get(m_jid) << "Not in roster => adding to rosterX cache";
		}
		else {
			if (hasFeature(GLOOX_FEATURE_ROSTERX)) {
				Log().Get(m_jid) << "Not in roster => sending rosterx";
				if (m_syncTimer==0){
					m_syncTimer = purple_timeout_add_seconds(4, sync_cb, this);
				}
				subscribeContact c;
				c.uin = name;
				c.alias = alias;
				c.group = (std::string) purple_group_get_name(purple_buddy_get_group(buddy));
				m_subscribeCache[name] = c;
			}
			else {
				Log().Get(m_jid) << "Not in roster => sending subscribe";
				Tag *tag = new Tag("presence");
				tag->addAttribute("type", "subscribe" );
				tag->addAttribute("from", name + "@" + p->jid());
				tag->addAttribute("to", m_jid);
				p->j->send(tag);
			}
		}
	}
	else {
		Tag *tag = generatePresenceStanza(buddy);
		if (tag){
			if (tag->xml()==m_roster[name].lastPresence){
				Log().Get(m_jid) << "Not sending this presence, because we've already sent it before.";
				delete tag;
				return;
			}
			else{
				m_roster[name].lastPresence=tag->xml();
			}
			tag->addAttribute( "to", m_jid );
			p->j->send( tag );
		}
	}
}

/*
 * Called when new message has been received.
 */
void User::purpleMessageReceived(PurpleAccount* account,char * name,char *msg,PurpleConversation *conv,PurpleMessageFlags flags){
	// new message grom legacy network has been received
	if (conv==NULL){
		// make conversation if it doesn't exist
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,account,name);
		m_conversations[(std::string)name]=conv;
	}
	// send message to user
	std::string message(purple_unescape_html(msg));
	Stanza *s = Stanza::createMessageStanza(m_jid, message);
	std::string from;
	from.append((std::string)name);
	from.append("@");
	from.append(p->jid()+"/bot");
	s->addAttribute("from",from);
	
	// chatstates
	if (hasFeature(GLOOX_FEATURE_CHATSTATES)){
		Tag *active = new Tag("active");
		active->addAttribute("xmlns","http://jabber.org/protocol/chatstates");
		s->addChild(active);
	}
	
	p->j->send( s );
}

/*
 * Called when legacy network user stops typing.
 */
void User::purpleBuddyTypingStopped(const std::string &uin){
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES))
		return;
	Log().Get(m_jid) << uin << " stopped typing";
	

	Tag *s = new Tag("message");
	s->addAttribute("to",m_jid);
	s->addAttribute("type","chat");
	
	std::string from;
	from.append(uin);
	from.append("@");
	from.append(p->jid()+"/bot");
	s->addAttribute("from",from);
	
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
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES))
		return;
	Log().Get(m_jid) << uin << " is typing";

	Tag *s = new Tag("message");
	s->addAttribute("to",m_jid);
	s->addAttribute("type","chat");
	
	std::string from;
	from.append(uin);
	from.append("@");
	from.append(p->jid()+"/bot");
	s->addAttribute("from",from);
	
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
	if (!hasFeature(GLOOX_FEATURE_CHATSTATES))
		return;
	Log().Get(m_jid) << "Sending " << state << " message to " << uin;
	if (state=="composing")
		serv_send_typing(purple_account_get_connection(m_account),uin.c_str(),PURPLE_TYPING);
	else if (state=="paused")
		serv_send_typing(purple_account_get_connection(m_account),uin.c_str(),PURPLE_TYPED);
	else
		serv_send_typing(purple_account_get_connection(m_account),uin.c_str(),PURPLE_NOT_TYPING);

}

/*
 * Received new message from jabber user.
 */
void User::receivedMessage( Stanza* stanza){
	PurpleConversation * conv;
	// open new conversation or get the opened one
	if (!isOpenedConversation(stanza->to().username())){
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,m_account,stanza->to().username().c_str());
		m_conversations[stanza->to().username()]=conv;
	}
	else{
		conv = m_conversations[stanza->to().username()];
	}
	// send this message
	PurpleConvIm *im = purple_conversation_get_im_data(conv);
	purple_conv_im_send(im,stanza->body().c_str());
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
	Log().Get(m_jid) <<  "purpleAuthorizeReceived: " << std::string(remote_user) << "on_list:" << on_list;
	// send subscribe presence to user
	Tag *tag = new Tag("presence");
	tag->addAttribute("type", "subscribe" );
	tag->addAttribute("from", (std::string)remote_user + "@" + p->jid());
	tag->addAttribute("to", m_jid);
	std::cout << tag->xml() << "\n";
	p->j->send(tag);

}

/*
 * Called when we're ready to connect (we know caps)
 */
void User::connect(){
	Log().Get(m_jid) << "Connecting with caps: " << m_capsVersion;
	if (purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str()) != NULL){
		Log().Get(m_jid) << "this account already exists";
		m_account = purple_accounts_find(m_username.c_str(), this->p->protocol()->protocol().c_str());
		purple_account_set_ui_bool(m_account,"hiicq","auto-login",false);
	}
	else{
		Log().Get(m_jid) << "creating new account";
		m_account = purple_account_new(m_username.c_str(), this->p->protocol()->protocol().c_str());
		purple_account_set_string(m_account,"encoding","windows-1250");
		purple_account_set_ui_bool(m_account,"hiicq","auto-login",false);

		purple_accounts_add(m_account);
	}
	m_connectionStart = time(NULL);
	m_readyForConnect = false;
	purple_account_set_string(m_account,"bind",std::string(m_bindIP).c_str());
	purple_account_set_string(m_account,"lastUsedJid",std::string(m_jid +"/"+m_resource).c_str());
	purple_account_set_password(m_account,m_password.c_str());
	Log().Get(m_jid) << "UIN:" << m_username << " PASSWORD:" << m_password;

	// check if it's valid uin
	bool valid = false;
	if (!m_username.empty()){
		valid = p->protocol()->isValidUsername(m_username);
	}
	if (valid){
		purple_account_set_enabled(m_account, HIICQ_UI, TRUE);
		purple_account_connect(m_account);
	}
}

/*
 * called when we are disconnected from legacy network
 */
void User::disconnected() {
	m_connected = false;
	m_reconnectCount += 1;
}

/*
 * called when we are disconnected from legacy network
 */
void User::connected() {
	m_connected = true;
	m_reconnectCount = 0;
}

/*
 * Received jabber presence...
 */
void User::receivedPresence(Stanza *stanza){
	// we're connected
	if (m_connected){
	
		// respond to probe presence
		if (stanza->subtype() == StanzaPresenceProbe && stanza->to().username()!=""){
			PurpleBuddy *buddy = purple_find_buddy(m_account, stanza->to().username().c_str());
			if (buddy){
				Tag *probe = generatePresenceStanza(buddy);
				if (probe){
					probe->addAttribute( "to", stanza->from().full() );
					p->j->send(probe);
				}
			}
// 				purpleBuddyChanged(buddy);
			else{
				Log().Get(m_jid) << "answering to probe presence with unavailable presence";
				Tag *tag = new Tag("presence");
				tag->addAttribute("to",stanza->from().full());
				tag->addAttribute("from",stanza->to().bare()+"/bot");
				tag->addAttribute("type","unavailable");
				p->j->send(tag);
			}
		}
	
		if(stanza->subtype() == StanzaS10nSubscribed) {
			// we've already received auth request from legacy server, so we should respond to this request
			if (hasAuthRequest((std::string)stanza->to().username())){
				Log().Get(m_jid) <<  "subscribed presence for authRequest arrived => accepting the request";
				m_authRequests[(std::string)stanza->to().username()].authorize_cb(m_authRequests[(std::string)stanza->to().username()].user_data);
				m_authRequests.erase(stanza->to().username());
			}
			// subscribed user is not in roster, so we must add him/her there.
			PurpleBuddy *buddy = purple_find_buddy(m_account, stanza->to().username().c_str());
			if(!isInRoster(stanza->to().username(),"both")) {
				if(!buddy) {
					Log().Get(m_jid) << "user is not in legacy network contact lists => nothing to be subscribed";
					// this user is not in ICQ contact list... It's nothing to do here,
					// because there is no user to authorize
				} else {
					Log().Get(m_jid) << "adding this user to local roster and sending presence";
					if (!isInRoster(stanza->to().username(),"ask")){
						// add user to mysql database and to local cache
						RosterRow user;
						user.id=-1;
						user.jid=m_jid;
						user.uin=stanza->to().username();
						user.subscription="both";
						user.online=false;
						user.lastPresence="";
						m_roster[stanza->to().username()]=user;
						p->sql()->addUserToRoster(m_jid,(std::string)stanza->to().username(),"both");
					}
					else{
						m_roster[stanza->to().username()].subscription="both";
						p->sql()->updateUserRosterSubscription(m_jid,(std::string)stanza->to().username(),"both");
					}
					// user is in ICQ contact list so we can inform jabber user
					// about status
					purpleBuddyChanged(buddy);
				}
			}
			// it can be reauthorization...
			if (buddy){
				purpleReauthorizeBuddy(buddy);
			}
			return;
		}
		else if(stanza->subtype() == StanzaS10nSubscribe) {
			PurpleBuddy *b = purple_find_buddy(m_account, stanza->to().username().c_str());
			if (b){
				purpleReauthorizeBuddy(b);
			}
			if(isInRoster(stanza->to().username(),"")) {
				if (m_roster[stanza->to().username()].subscription=="ask") {
					Tag *reply = new Tag("presence");
					reply->addAttribute( "to", stanza->from().bare() );
					reply->addAttribute( "from", stanza->to().bare() );
					reply->addAttribute( "type", "subscribe" );
					p->j->send( reply );
				}
				Log().Get(m_jid) << "subscribe presence; user is already in roster => sending subscribed";
				// username is in local roster (he/her is in ICQ roster too),
				// so we should send subscribed presence
				Tag *reply = new Tag("presence");
				reply->addAttribute( "to", stanza->from().bare() );
				reply->addAttribute( "from", stanza->to().bare() );
				reply->addAttribute( "type", "subscribed" );
				p->j->send( reply );
			}
			else{
				Log().Get(m_jid) << "subscribe presence; user is not in roster => adding to legacy network";
				// this user is not in local roster, so we have to send add_buddy request
				// to our legacy network and wait for response
				PurpleBuddy *buddy = purple_buddy_new(m_account,stanza->to().username().c_str(),stanza->to().username().c_str());
				purple_blist_add_buddy(buddy, NULL, NULL ,NULL);
				purple_account_add_buddy(m_account, buddy);
			}
			return;
		} else if(stanza->subtype() == StanzaS10nUnsubscribe || stanza->subtype() == StanzaS10nUnsubscribed) {
			PurpleBuddy *buddy = purple_find_buddy(m_account, stanza->to().username().c_str());
			if(stanza->subtype() == StanzaS10nUnsubscribed) {
				// user respond to auth request from legacy network and deny it
				if (hasAuthRequest((std::string)stanza->to().username())){
					Log().Get(m_jid) << "unsubscribed presence for authRequest arrived => rejecting the request";
					m_authRequests[(std::string)stanza->to().username()].deny_cb(m_authRequests[(std::string)stanza->to().username()].user_data);
					m_authRequests.erase(stanza->to().username());
					return;
				}
			}
			if(buddy && isInRoster(stanza->to().username(),"both")) {
				Log().Get(m_jid) << "unsubscribed presence => removing this contact from legacy network";
				// thi contact is in ICQ contact list, so we can remove him/her
				purple_account_remove_buddy(m_account,buddy,purple_buddy_get_group(buddy));
				purple_blist_remove_buddy(buddy);
// 				purple_privacy_permit_remove(m_account, stanza->to().username().c_str(),false);
			} else {
				// this contact is not in ICQ contact list, so there is nothing to remove...
			}
			if(isInRoster(stanza->to().username(),"both")) {
				// this contact is in our local roster, so we have to remove her/him
				Log().Get(m_jid) << "removing this contact from local roster";
				m_roster.erase(stanza->to().username());
				p->sql()->removeUINFromRoster(m_jid,stanza->to().username());
			}
			// inform user about removing this contact
			Tag *tag = new Tag("presence");
			tag->addAttribute( "to", stanza->from().bare() );
			tag->addAttribute( "from", stanza->to().username() + "@" + p->jid()+"/bot" );
			if(stanza->subtype() == StanzaS10nUnsubscribe) {
				tag->addAttribute( "type", "unsubscribe" );
			} else {
				tag->addAttribute( "type", "unsubscribed" );
			}
			p->j->send( tag );
			return;
		}
	}

	// this presence is for the transport
	if(stanza->to().username() == ""){
		if(stanza->presence() == PresenceUnavailable) {
			// disconnect from legacy network if we are connected
			std::map<std::string,int> ::iterator iter = m_resources.begin();
			if ((m_connected==false && int(time(NULL))>int(m_connectionStart)+10) || m_connected==true){
				iter = m_resources.find(stanza->from().resource());
				if(iter != m_resources.end()){
					m_resources.erase(stanza->from().resource());
				}
			}
			if (m_connected){
				if (m_resources.empty()){
					Log().Get(m_jid) << "disconecting";
					purple_account_disconnect(m_account);
				}
				else{

					iter = m_resources.begin();
					m_resource=(*iter).first;
				}
// 				m_connected=false;
			}
			else {
				if (!m_resources.empty() && int(time(NULL))>int(m_connectionStart)+10){
					iter = m_resources.begin();
					m_resource=(*iter).first;
				}
				else if (m_account){
					Log().Get(m_jid) << "disconecting2";
					purple_account_disconnect(m_account);
				}
			}
		} else if(stanza->subtype() == StanzaPresenceAvailable) {
			m_resource=stanza->from().resource();
			std::map<std::string,int> ::iterator iter = m_resources.begin();
			iter = m_resources.find(m_resource);
			if(iter == m_resources.end()){
				m_resources[m_resource]=stanza->priority();
			}

			Log().Get(m_jid) << "resource: " << m_resource;
			if (!m_connected){
				// we are not connected to legacy network, so we should do it when disco#info arrive :)
				Log().Get(m_jid) << "connecting: capsVersion=" << m_capsVersion;
				if (m_readyForConnect==false){
					m_readyForConnect=true;
					if (m_capsVersion.empty()){
						// caps not arrived yet, so we can't connect just now and we have to wait for caps
					}
					else{
						connect();
					}
				}
			}
			else {
				Log().Get(m_jid) << "mirroring presence to legacy network";
				// we are already connected so we have to change status
				PurpleSavedStatus *status;
				int PurplePresenceType;
				std::string statusMessage;
				
				// mirror presence types
				switch(stanza->presence()) {
					case PresenceAvailable: {
						PurplePresenceType=PURPLE_STATUS_AVAILABLE;
						break;
					}
					case PresenceChat: {
						PurplePresenceType=PURPLE_STATUS_AVAILABLE;
						break;
					}
					case PresenceAway: {
						PurplePresenceType=PURPLE_STATUS_AWAY;
						break;
					}
					case PresenceDnd: {
						PurplePresenceType=PURPLE_STATUS_UNAVAILABLE;
						break;
					}
					case PresenceXa: {
						PurplePresenceType=PURPLE_STATUS_EXTENDED_AWAY;
						break;
					}
				}
				// send presence to our legacy network
				status = purple_savedstatus_new(NULL, (PurpleStatusPrimitive)PurplePresenceType);

				statusMessage.clear();

				if (hasTransportFeature(TRANSPORT_MANGLE_STATUS)) {
					p->sql()->getRandomStatus(statusMessage);
                    statusMessage.append(" - ");
				}

				statusMessage.append(stanza->status());

				if (!statusMessage.empty())
					purple_savedstatus_set_message(status,statusMessage.c_str());
				purple_savedstatus_activate_for_account(status,m_account);
			}
			
		}
		
		// send presence about tranport status to user
		if(m_connected || m_readyForConnect) {
			Stanza *tag = Stanza::createPresenceStanza(m_jid, stanza->status(),stanza->presence());
			tag->addAttribute( "from", p->jid() );
			p->j->send( tag );
		} else {
			if (m_resource.empty()){
				Tag *tag = new Tag("presence");
				tag->addAttribute( "to", stanza->from().bare() );
				tag->addAttribute( "type", "unavailable" );
				tag->addAttribute( "from", p->jid() );
				p->j->send( tag );
			}
		}
	}
}

User::~User(){

	// send unavailable to online users
	Tag *tag;
	for(std::map<std::string, RosterRow>::iterator u = m_roster.begin(); u != m_roster.end() ; u++){
		if ((*u).second.online==true){
			tag = new Tag("presence");
			tag->addAttribute( "to", m_jid );
			tag->addAttribute( "type", "unavailable" );
			tag->addAttribute( "from", (*u).second.uin+"@"+ p->jid() + "/bot");
			p->j->send( tag );
		}
	}

	// destroy conversations
// 	GList *l;
// 	for (l = purple_get_conversations(); l != NULL; l = l->next){
// 		PurpleConversation *conv = (PurpleConversation *)l->data;
// 		if (purple_conversation_get_account(conv) == m_account){
// 			purple_conversation_destroy(conv);
// 		}
// 	}
	GList *iter;
	for (iter = purple_get_conversations(); iter; ) {
			PurpleConversation *conv = (PurpleConversation*) iter->data;
			iter = iter->next;
			if (purple_conversation_get_account(conv) == m_account)
				purple_conversation_destroy(conv);
	}



// 	purple_account_destroy(m_account);
// 	delete(m_account);
// 	if (this->save_timer!=0 && this->save_timer!=-1)
// 		std::cout << "* removing timer\n";
// 		purple_timeout_remove(this->save_timer);
	if (m_syncTimer!=0) {
		Log().Get(m_jid) << "removing timer\n";
		purple_timeout_remove(m_syncTimer);
	}
	m_roster.clear();
	m_conversations.clear();
	m_authRequests.clear();
}


