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

#include "caps.h"
#include <gloox/clientbase.h>
#include <glib.h>
#include "main.h"
#include "sql.h"
#include "usermanager.h"

GlooxDiscoHandler::GlooxDiscoHandler(GlooxMessageHandler *parent) : DiscoHandler(){
	p=parent;
	version = 0;
}

GlooxDiscoHandler::~GlooxDiscoHandler(){
}

bool GlooxDiscoHandler::hasVersion(int name){
	std::map<int,std::string> ::iterator iter = versions.begin();
	iter = versions.find(name);
	if(iter != versions.end())
		return true;
	return false;
}

// 	void handleDiscoInfo(const JID &jid, const Disco::Info &info, int context);
// 	void handleDiscoItems(const JID &jid, const Disco::Items &items, int context);
// 	void handleDiscoError(const JID &jid, const Error *error, int context);

void GlooxDiscoHandler::handleDiscoInfo(const JID &jid, const Disco::Info &info, int context) {
// 	if (stanza->id().empty())
// 		return;
	if (!hasVersion(context))
		return;
// 	Tag *query = stanza->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
// 	if (query==NULL)
// 		return;
	//if (query->findChild("identity") && query->findChild("identity")->findChild("category") && !query->findChild("identity")->findChildWithAttrib("category","client"))
		//return;
	Tag *query = info.tag();
	if (query->findChild("identity") && !query->findChildWithAttrib("category","client"))
		return;
	int feature=0;
	std::list<Tag*> features = query->findChildren("feature");
		for (std::list<Tag*>::const_iterator it = features.begin(); it != features.end(); ++it) {
// 			std::cout << *it << std::endl;
			if ((*it)->findAttribute("var")=="http://jabber.org/protocol/rosterx"){
				feature=feature|GLOOX_FEATURE_ROSTERX;
			}
			else if ((*it)->findAttribute("var")=="http://jabber.org/protocol/xhtml-im"){
				feature=feature|GLOOX_FEATURE_XHTML_IM;
			}
			else if ((*it)->findAttribute("var")=="http://jabber.org/protocol/si/profile/file-transfer"){
				feature=feature|GLOOX_FEATURE_FILETRANSFER;
			}
			else if ((*it)->findAttribute("var")=="http://jabber.org/protocol/chatstates"){
				feature=feature|GLOOX_FEATURE_CHATSTATES;
			}
		}
	std::cout << "*** FEATURES ARRIVED: " << feature << "\n";
	p->capsCache[versions[context]]=feature;
	User *user = p->userManager()->getUserByJID(jid.bare());
	if (user==NULL){
		std::cout << "no user?! wtf...";
	}
	else{
		if (user->capsVersion().empty()){
			user->setCapsVersion(versions[context]);
			if (user->readyForConnect())
				user->connect();
		}
	}
	// TODO: CACHE CAPS IN DATABASE ACCORDING TO VERSION
	versions.erase(context);
}

void GlooxDiscoHandler::handleDiscoItems(const JID &jid, const Disco::Items &items, int context){

}

void GlooxDiscoHandler::handleDiscoError(const JID &jid, const Error *error, int context){
// // 	if (stanza->id().empty())
// // 		return;
// // 	if (!hasVersion(context))
// // 		return;
// // 	Tag *query = stanza->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
// // 	if (query==NULL)
// // 		return;
// 	// we are now using timeout
// 	return;
// 	int feature=0;
// 	p->capsCache[versions[stanza->id()]]=feature;
// 	std::cout << "*** FEATURES ERROR received: " << feature << "\n";
// 	User *user = p->userManager()->getUserByJID(stanza->from().bare());
// 	if (user==NULL){
// 		std::cout << "no user?! wtf...";
// 	}
// 	else{
// 		if (user->capsVersion().empty()){
// 			user->setCapsVersion(versions[stanza->id()]);
// 			if (user->readyForConnect())
// 				user->connect();
// 		}
// 	}
// 	versions.erase(stanza->id());
}
