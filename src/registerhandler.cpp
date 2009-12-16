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

#include "registerhandler.h"
#include "main.h"
#include <gloox/clientbase.h>
#include <gloox/registration.h>
#include <glib.h>
#include "sql.h"
#include "caps.h"
#include "usermanager.h"
#include "accountcollector.h"
#include "protocols/abstractprotocol.h"
#include "user.h"

GlooxRegisterHandler::GlooxRegisterHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
	p->j->registerStanzaExtension( new Registration::Query() );
}

GlooxRegisterHandler::~GlooxRegisterHandler(){
}

bool GlooxRegisterHandler::handleIq (const IQ &iq){
	Log("GlooxRegisterHandler", iq.from().full() << ": iq:register received (" << iq.subtype() << ")");
	User *user = (User *) p->userManager()->getUserByJID(iq.from().bare());
	if (p->configuration().onlyForVIP){
		std::list<std::string> const &x = p->configuration().allowedServers;
		if (std::find(x.begin(), x.end(), iq.from().server()) == x.end()) {
			Log("GlooxRegisterHandler", "This user has no permissions to register an account");
			sendError(400, "bad-request", iq);
			return false;
		}
	}


	// send registration form
	if(iq.subtype() == IQ::Get) {
		Tag *reply = new Tag( "iq" );
		reply->addAttribute( "id", iq.id() );
		reply->addAttribute( "type", "result" );
		reply->addAttribute( "to", iq.from().full() );
		reply->addAttribute( "from", p->jid() );
		Tag *query = new Tag( "query" );
		query->addAttribute( "xmlns", "jabber:iq:register" );
		UserRow res = p->sql()->getUserByJid(iq.from().bare());

		if (res.id == -1) {
			Log("GlooxRegisterHandler", "* sending registration form; user is not registered");
			query->addChild( new Tag("instructions", p->protocol()->text("instructions")) );
			query->addChild( new Tag("username") );
			query->addChild( new Tag("password") );
		}
		else {
			Log("GlooxRegisterHandler", "* sending registration form; user is registered");
			query->addChild( new Tag("instructions", p->protocol()->text("instructions")) );
			query->addChild( new Tag("registered") );
			query->addChild( new Tag("username",res.uin));
			query->addChild( new Tag("password"));
		}

		Tag *x = new Tag("x");
		x->addAttribute("xmlns", "jabber:x:data");
		x->addAttribute("type", "form");
		x->addChild( new Tag("title", "Registration") );
		x->addChild( new Tag("instructions", p->protocol()->text("instructions")) );

		Tag *field = new Tag("field");
		field->addAttribute("type", "hidden");
		field->addAttribute("var", "FORM_TYPE");
		field->addChild( new Tag("value", "jabber:iq:register") );
		x->addChild(field);

		field = new Tag("field");
		field->addAttribute("type", "text-single");
		field->addAttribute("var", "username");
		field->addAttribute("label", "Network username");
		field->addChild( new Tag("required") );
		if (res.id!=-1)
			field->addChild( new Tag("value", res.uin) );
		x->addChild(field);

		field = new Tag("field");
		field->addAttribute("type", "text-private");
		field->addAttribute("var", "password");
		field->addAttribute("label", "Password");
		x->addChild(field);

		field = new Tag("field");
		field->addAttribute("type", "list-single");
		field->addAttribute("var", "language");
		field->addAttribute("label", "Language");
		if (res.id!=-1)
			field->addChild( new Tag("value", res.language) );
		else
			field->addChild( new Tag("value", p->configuration().language) );
		x->addChild(field);

		Tag *option = new Tag("option");
		option->addAttribute("label", "Cesky");
		option->addChild( new Tag("value", "cs") );
		field->addChild(option);

		option = new Tag("option");
		option->addAttribute("label", "English");
		option->addChild( new Tag("value", "en") );
		field->addChild(option);

		if (res.id != -1) {
			field = new Tag("field");
			field->addAttribute("type", "boolean");
			field->addAttribute("var", "unregister");
			field->addAttribute("label", "Unregister transport");
			field->addChild( new Tag("value", "0") );
			x->addChild(field);
		}

		query->addChild(x);
		reply->addChild(query);
		p->j->send( reply );
		return true;
	}
	else if(iq.subtype() == IQ::Set) {
		bool sendsubscribe = false;
		bool remove = false;
		Tag *iqTag = iq.tag();
		Tag *query;
		Tag *usernametag;
		Tag *passwordtag;
		Tag *languagetag;
		std::string username("");
		std::string password("");
		std::string language("");

		UserRow res = p->sql()->getUserByJid(iq.from().bare());
		
		query = iqTag->findChild( "query" );
		if (!query) return true;

		Tag *xdata = query->findChild("x", "xmlns", "jabber:x:data");
		if (xdata) {
			if (query->hasChild( "remove" ))
				remove = true;
			for (std::list<Tag*>::const_iterator it = xdata->children().begin(); it != xdata->children().end(); ++it) {
				std::string key = (*it)->findAttribute("var");
				if (key.empty()) continue;

				Tag *v = (*it)->findChild("value");
				if (!v) continue;

				if (key == "username")
					username = v->cdata();
				else if (key == "password")
					password = v->cdata();
				else if (key == "language")
					language = v->cdata();
				else if (key == "unregister")
					remove = atoi(v->cdata().c_str());
			}
		}
		else {
			if (query->hasChild( "remove" ))
				remove = true;
			usernametag = query->findChild("username");
			passwordtag = query->findChild("password");
			languagetag = query->findChild("language");

			if (languagetag)
				language = languagetag->cdata();
			else
				language = p->configuration().language;

			if (usernametag==NULL || passwordtag==NULL) {
				sendError(406, "not-acceptable", iq);
				return false;
			}
			else {
				username = usernametag->cdata();
				password = passwordtag->cdata();
			}
		}

		if (remove) {
			Log("GlooxRegisterHandler", "removing user from database and disconnecting from legacy network");
			PurpleAccount *account = NULL;
			if (user != NULL) {
				if (user->isConnected()) {
					account = user->account();
					purple_account_disconnect(user->account());
					user->disconnected();
				}
			}
			// TODO: uncomment this line when jabbim will know rosterx
			//if (user->hasFeature(GLOOX_FEATURE_ROSTERX)){
			if (true){
				std::cout << "* sending rosterX\n";
				Tag *tag = new Tag("message");
				tag->addAttribute( "to", iq.from().bare() );
				std::string from;
				from.append(p->jid());
				tag->addAttribute( "from", from );
				tag->addChild(new Tag("body","removing users"));
				Tag *x = new Tag("x");
				x->addAttribute("xmlns","http://jabber.org/protocol/rosterx");

				std::map<std::string,RosterRow> roster;
// 				roster = p->sql()->getBuddies(res.id);
// 				Tag *item;
// 				// add users which are added to roster
// 				for(std::map<std::string, RosterRow>::iterator u = roster.begin(); u != roster.end() ; u++){
// 					if (!(*u).second.uin.empty()){
// 						item = new Tag("item");
// 						item->addAttribute("action","delete");
// 						item->addAttribute("jid",(*u).second.uin+"@"+p->jid());
// 						x->addChild(item);
// 					}
// 				}

				tag->addChild(x);
				std::cout << "* sending " << tag->xml() << "\n";
				p->j->send(tag);
				if (res.id != -1) {
					p->sql()->removeUser(iq.from().bare());
					p->sql()->removeUserBuddies(res.id);
				}
// 				if (account)
// 					p->collector()->collectNow(account, true);
			}
			else{
				// TODO: remove contacts from roster with unsubscribe presence
// 					for(std::map<std::string, RosterRow>::iterator u = user->roster.begin(); u != user->roster.end() ; u++){
// 						item = new Tag("item");
// 						item->addAttribute("action","delete");
// 						item->addAttribute("jid",(*u).uin+"@"+p->jid());
// 						x->addChild(item);
// 					}
			}

			if (user != NULL) {
				p->userManager()->removeUser(user);
			}
			Tag *reply = new Tag("iq");
			reply->addAttribute( "type", "result" );
			reply->addAttribute( "from", p->jid() );
			reply->addAttribute( "to", iq.from().full() );
			reply->addAttribute( "id", iq.id() );
			p->j->send( reply );

			reply = new Tag( "presence" );
			reply->addAttribute( "type", "unsubscribe" );
			reply->addAttribute( "from", p->jid() );
			reply->addAttribute( "to", iq.from().full() );
			p->j->send( reply );

			reply = new Tag("presence");
			reply->addAttribute( "type", "unsubscribed" );
			reply->addAttribute( "to", iq.from().full() );
			reply->addAttribute( "from", p->jid() );
			p->j->send( reply );

			delete iqTag;
			return true;
		}

		// Register or change password

		std::string jid = iq.from().bare();

		if (username.empty() || password.empty()) {
			sendError(406, "not-acceptable", iq);
			return false;
		}

		p->protocol()->prepareUserName(username);
		if (!p->protocol()->isValidUsername(username)) {
			Log("GlooxRegisterHandler", "This is now valid username: "<< username);
			sendError(400, "bad-request", iq);
			return false;
		}

		if (res.id == -1) {
			Log("GlooxRegisterHandler", "adding new user: "<< jid << ", " << username << ", " << password << ", " << language);
			p->sql()->addUser(jid,username,password,language);
			sendsubscribe = true;
		}
		else {
			// change passwordhttp://soumar.jabbim.cz/phpmyadmin/index.php
			Log("GlooxRegisterHandler", "changing user password: "<< jid << ", " << username << ", " << password);
			p->sql()->updateUserPassword(iq.from().bare(),password,language);
		}

		Tag *reply = new Tag( "iq" );
		reply->addAttribute( "id", iq.id() );
		reply->addAttribute( "type", "result" );
		reply->addAttribute( "to", iq.from().full() );
		reply->addAttribute( "from", p->jid() );
		Tag *rquery = new Tag( "query" );
		rquery->addAttribute( "xmlns", "jabber:iq:register" );
		reply->addChild(rquery);
		p->j->send( reply );

		if (sendsubscribe) {
			reply = new Tag("presence");
			reply->addAttribute( "from", p->jid() );
			reply->addAttribute( "to", iq.from().bare() );
			reply->addAttribute( "type", "subscribe" );
			p->j->send( reply );
		}
		delete iqTag;
		return true;
	}
	return false;
}

void GlooxRegisterHandler::handleIqID (const IQ &iq, int context){
	std::cout << "IQ ID IQ ID IQ ID\n";
}

void GlooxRegisterHandler::sendError(int code, const std::string &err, const IQ &iq) {
	Tag *iq2 = new Tag("iq");
	iq2->addAttribute("type","error");
	iq2->addAttribute("from", p->jid());
	iq2->addAttribute("to", iq.from().full());
	iq2->addAttribute("id", iq.id());

	Tag *error = new Tag("error");
	error->addAttribute("code",code);
	error->addAttribute("type","modify");
	Tag *bad = new Tag(err);
	bad->addAttribute("xmlns","urn:ietf:params:xml:ns:xmpp-stanzas");

	error->addChild(bad);
	iq2->addChild(error);

	p->j->send(iq2);
}

