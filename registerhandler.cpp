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
#include "protocols/abstractprotocol.h"

GlooxRegisterHandler::GlooxRegisterHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
	p->j->registerStanzaExtension( new Registration::Query() );
}

GlooxRegisterHandler::~GlooxRegisterHandler(){
}

bool GlooxRegisterHandler::handleIq (const IQ &iq){
	std::cout << "*** "<< iq.from().full() << ": iq:register received (" << iq.subtype() << ")\n";
	User *user = p->userManager()->getUserByJID(iq.from().bare());
	if (p->configuration().onlyForVIP){
		bool isVIP = p->sql()->isVIP(iq.from().bare());
		std::list<std::string> const &x = p->configuration().allowedServers;
		if (!isVIP && std::find(x.begin(), x.end(), iq.from().server()) == x.end())
			return false;
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
		if(res.id==-1) {
			std::cout << "* sending registration form; user is not registered\n";
			query->addChild( new Tag("instructions", p->protocol()->text("instructions")) );
			query->addChild( new Tag("username") );
			query->addChild( new Tag("password") );
			query->addChild( new Tag("language", "cs") );
		}
		else {
			std::cout << "* sending registration form; user is registered\n";
			query->addChild( new Tag("instructions", p->protocol()->text("instructions")) );
			query->addChild( new Tag("registered") );
			query->addChild( new Tag("username",res.uin));
			query->addChild( new Tag("password"));
			query->addChild( new Tag("language", res.language) );
		}
	
// 		Tag *x = new Tag("x");
// 		x->addAttribute("xmlns", "jabber:x:data");
// 		x->addAttribute("type", "form");
// 		x->addChild( new Tag("title","Registration") )
// 		x->addChild( new Tag("instructions", p->protocol()->text("instructions")) );
// 		<field type='text-single' label='The name of your bot' var='botname'/>
		
		
		reply->addChild(query);
		p->j->send( reply );
		return true;
	}
	else if(iq.subtype() == IQ::Set) {
		bool sendsubscribe = false;
		Tag *iqTag = iq.tag();
		Tag *query = iqTag->findChild( "query" );

		if (!query) return true;

		// someone wants to be removed
		if(query->hasChild( "remove" )) {
			std::cout << "* removing user from database and disconnecting from legacy network\n";
			if (user!=NULL){
				if (user->isConnected()==true){
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
				Tag *item;

				std::map<std::string,RosterRow> roster;
				roster = p->sql()->getRosterByJid(iq.from().bare());
				// add users which are added to roster
				for(std::map<std::string, RosterRow>::iterator u = roster.begin(); u != roster.end() ; u++){
					if (!(*u).second.uin.empty()){
						item = new Tag("item");
						item->addAttribute("action","delete");
						item->addAttribute("jid",(*u).second.uin+"@"+p->jid());
						x->addChild(item);
					}
				}

				tag->addChild(x);
				std::cout << "* sending " << tag->xml() << "\n";
				p->j->send(tag);
				p->sql()->removeUser(iq.from().bare());
				p->sql()->removeUserFromRoster(iq.from().bare());

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

			if (user!=NULL){
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
		Tag *usernametag = query->findChild("username");
		Tag *passwordtag = query->findChild("password");
		Tag *languagetag = query->findChild("password");
		std::string username("");
		std::string password("");
		std::string language("");
		
		if (languagetag)
			language = languagetag->cdata();
		
		bool e = false;
		if (usernametag==NULL || passwordtag==NULL)
			e=true;
		else {
			username = usernametag->cdata();
			password = passwordtag->cdata();
		}

		if (username.empty() || password.empty())
			e=true;
		
		username = p->protocol()->prepareUserName(username);
		if (!p->protocol()->isValidUsername(username))
			e = true;
		
//    <iq type='error' from='shakespeare.lit' to='bill@shakespeare.lit/globe' id='change1'>
//        <error code='400' type='modify'>
//    	    <bad-request xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>
//	</error>
//    </iq>
		if (e) {
		    Tag *iq2 = new Tag("iq");
		    iq2->addAttribute("type","error");
		    iq2->addAttribute("from", p->jid());
		    iq2->addAttribute("to", iq.from().full());
		    iq2->addAttribute("id", iq.id());
		    
			Tag *error = new Tag("error");
		    error->addAttribute("code",400);
		    error->addAttribute("type","modify");
		    Tag *bad = new Tag("bad-request");
		    bad->addAttribute("xmlns","urn:ietf:params:xml:ns:xmpp-stanzas");
		    
		    error->addChild(bad);
		    iq2->addChild(error);
		    
		    p->j->send(iq2);
		    
			delete iqTag;
		    return true;
		}



		UserRow res = p->sql()->getUserByJid(iq.from().bare());
		if(res.id==-1) {
	
// 			if(false) {
// 			XDEBUG(( "User already exists .. :( \n" ));
// 			Stanza *r = new Stanza(stanza);//->clone();
// 			Tag *error = new Tag("error");
// 			error->addAttribute( "code", "409" );
// 			error->addAttribute( "type", "cancel" );
// 			Tag *conflict = new Tag("conflict");
// 			conflict->addAttribute( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
// 			conflict->setCData( "Someone else already registered with this xfire username (" + user->jid + ")" );
// 			error->addChild( conflict );
// 			r->addChild( error );
// 			comp->j->send( r );
// 		} else {
	
			std::cout << "* adding new user: "<< jid << ", " << username << ", " << password <<"\n";
			p->sql()->addUser(jid,username,password,language);
			sendsubscribe = true;
		} else {
			// change password
			std::cout << "* changing user password: "<< jid << ", " << username << ", " << password <<"\n";
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
		
		if(sendsubscribe) {
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
