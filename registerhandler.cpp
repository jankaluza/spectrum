#include "registerhandler.h"
#include "main.h"
#include <gloox/clientbase.h>
#include <glib.h>
#include "sql.h"
#include "caps.h"
#include "usermanager.h"
#include "protocols/abstractprotocol.h"

GlooxRegisterHandler::GlooxRegisterHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
}

GlooxRegisterHandler::~GlooxRegisterHandler(){
}

bool GlooxRegisterHandler::handleIq (Stanza *stanza){
	std::cout << "*** "<< stanza->from().full() << ": iq:register received (" << stanza->subtype() << ")\n";
	User *user = p->userManager()->getUserByJID(stanza->from().bare());
	if (p->configuration().onlyForVIP){
		bool isVIP = p->sql()->isVIP(stanza->from().bare());
		if (!isVIP)
			return false;
	}
	
	
	// send registration form
	if(stanza->subtype() == StanzaIqGet) {
		Tag *reply = new Tag( "iq" );
		reply->addAttribute( "id", stanza->id() );
		reply->addAttribute( "type", "result" );
		reply->addAttribute( "to", stanza->from().full() );
		reply->addAttribute( "from", p->jid() );
		Tag *query = new Tag( "query" );
		query->addAttribute( "xmlns", "jabber:iq:register" );
		query->addChild( new Tag("instructions", "Enter your UIN and password") );
		UserRow res = p->sql()->getUserByJid(stanza->from().bare());
		if(res.id==-1) {
			std::cout << "* sending registration form; user is not registered\n";
			query->addChild( new Tag("instructions", "Enter your UIN and password") );
			query->addChild( new Tag("username") );
			query->addChild( new Tag("password") );
		}
		else {
			std::cout << "* sending registration form; user is registered\n";
			query->addChild( new Tag("instructions", "Enter your UIN and password") );
			query->addChild( new Tag("registered") );
			query->addChild( new Tag("username",res.uin));
			query->addChild( new Tag("password"));
		}
		
		reply->addChild(query);
		p->j->send( reply );
		return true;
	}
	else if(stanza->subtype() == StanzaIqSet) {
		bool sendsubscribe = false;
		Tag *query = stanza->findChild( "query" );

		if (!query) return true;

		// someone wants to be removed
		if(query->hasChild( "remove" )) {
			std::cout << "* removing user from database and disconnecting from legacy network\n";
			if (user!=NULL){
				if (user->connected==true){
					purple_account_disconnect(user->account());
					user->connected=false;
				}
			}
			// TODO: uncomment this line when jabbim will know rosterx
			//if (user->hasFeature(GLOOX_FEATURE_ROSTERX)){
			if (true){
				std::cout << "* sending rosterX\n";
				Tag *tag = new Tag("message");
				tag->addAttribute( "to", stanza->from().bare() );
				std::string from;
				from.append(p->jid());
				tag->addAttribute( "from", from );
				tag->addChild(new Tag("body","removing users"));
				Tag *x = new Tag("x");
				x->addAttribute("xmlns","http://jabber.org/protocol/rosterx");
				Tag *item;

				std::map<std::string,RosterRow> roster;
				roster = p->sql()->getRosterByJid(stanza->from().bare());
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
				p->sql()->removeUser(stanza->from().bare());
				p->sql()->removeUserFromRoster(stanza->from().bare());

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
			reply->addAttribute( "to", stanza->from().full() );
			reply->addAttribute( "id", stanza->id() );
			p->j->send( reply );

			reply = new Tag( "presence" );
			reply->addAttribute( "type", "unsubscribe" );
			reply->addAttribute( "from", p->jid() );
			reply->addAttribute( "to", stanza->from().full() );
			p->j->send( reply );

			reply = new Tag("presence");
			reply->addAttribute( "type", "unsubscribed" );
			reply->addAttribute( "to", stanza->from().full() );
			reply->addAttribute( "from", p->jid() );
			p->j->send( reply );
			return true;
		}
	
		// Register or change password
	
		std::string jid = stanza->from().bare();
		Tag *usernametag = query->findChild("username");
		Tag *passwordtag = query->findChild("password");
		std::string username("");
		std::string password("");
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
		    Tag *iq = new Tag("iq");
		    iq->addAttribute("type","error");
		    iq->addAttribute("from", p->jid());
		    iq->addAttribute("to", stanza->from().full());
		    iq->addAttribute("id", stanza->id());
		    
			Tag *error = new Tag("error");
		    error->addAttribute("code",400);
		    error->addAttribute("type","modify");
		    Tag *bad = new Tag("bad-request");
		    bad->addAttribute("xmlns","urn:ietf:params:xml:ns:xmpp-stanzas");
		    
		    error->addChild(bad);
		    iq->addChild(error);
		    
		    p->j->send(iq);
		    
		    return true;
		}



		UserRow res = p->sql()->getUserByJid(stanza->from().bare());
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
			p->sql()->addUser(jid,username,password);
			sendsubscribe = true;
		} else {
			// change password
			std::cout << "* changing user password: "<< jid << ", " << username << ", " << password <<"\n";
			p->sql()->updateUserPassword(stanza->from().bare(),password);
		}
		Tag *reply = new Tag( "iq" );
		reply->addAttribute( "id", stanza->id() );
		reply->addAttribute( "type", "result" );
		reply->addAttribute( "to", stanza->from().full() );
		reply->addAttribute( "from", p->jid() );
		Tag *rquery = new Tag( "query" );
		rquery->addAttribute( "xmlns", "jabber:iq:register" );
		reply->addChild(rquery);
		p->j->send( reply );
		
		if(sendsubscribe) {
		reply = new Tag("presence");
		reply->addAttribute( "from", p->jid() );
		reply->addAttribute( "to", stanza->from().bare() );
		reply->addAttribute( "type", "subscribe" );
		p->j->send( reply );
		}
		return true;
	}
	return false;
}

bool GlooxRegisterHandler::handleIqID (Stanza *stanza, int context){
	std::cout << "IQ ID IQ ID IQ ID\n";
	return true;
}
