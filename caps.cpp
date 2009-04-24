#include "caps.h"
#include <gloox/clientbase.h>
#include <glib.h>
#include "main.h"
#include "sql.h"
#include "usermanager.h"

GlooxDiscoHandler::GlooxDiscoHandler(GlooxMessageHandler *parent) : DiscoHandler(){
	p=parent;
}

GlooxDiscoHandler::~GlooxDiscoHandler(){
}

bool GlooxDiscoHandler::hasVersion(const std::string &name){
	std::map<std::string,std::string> ::iterator iter = versions.begin();
	iter = versions.find(name);
	if(iter != versions.end())
		return true;
	return false;
}

void GlooxDiscoHandler::handleDiscoInfoResult(Stanza *stanza,int context){
	if (stanza->id().empty())
		return;
	if (!hasVersion(stanza->id()))
		return;
	Tag *query = stanza->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
	if (query==NULL)
		return;
	//if (query->findChild("identity") && query->findChild("identity")->findChild("category") && !query->findChild("identity")->findChildWithAttrib("category","client"))
		//return;
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
	p->capsCache[versions[stanza->id()]]=feature;
	User *user = p->userManager()->getUserByJID(stanza->from().bare());
	if (user==NULL){
		std::cout << "no user?! wtf...";
	}
	else{
		if (user->capsVersion.empty()){
			user->capsVersion=versions[stanza->id()];
			if (user->readyForConnect)
				user->connect();
		}
	}
	// TODO: CACHE CAPS IN DATABASE ACCORDING TO VERSION
	versions.erase(stanza->id());
}

void GlooxDiscoHandler::handleDiscoItemsResult(Stanza *stanza,int context){

}

void GlooxDiscoHandler::handleDiscoError(Stanza *stanza,int context){
	if (stanza->id().empty())
		return;
	if (!hasVersion(stanza->id()))
		return;
	Tag *query = stanza->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
	if (query==NULL)
		return;
	// we are now using timeout
	return;
	int feature=0;
	p->capsCache[versions[stanza->id()]]=feature;
	std::cout << "*** FEATURES ERROR received: " << feature << "\n";
	User *user = p->userManager()->getUserByJID(stanza->from().bare());
	if (user==NULL){
		std::cout << "no user?! wtf...";
	}
	else{
		if (user->capsVersion.empty()){
			user->capsVersion=versions[stanza->id()];
			if (user->readyForConnect)
				user->connect();
		}
	}
	versions.erase(stanza->id());
}
