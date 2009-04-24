#include "discoinfohandler.h"
#include "protocols/abstractprotocol.h"

GlooxDiscoInfoHandler::GlooxDiscoInfoHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
}

GlooxDiscoInfoHandler::~GlooxDiscoInfoHandler(){
}


bool GlooxDiscoInfoHandler::handleIq (Stanza *stanza){

/*	User *user = p->getUserByJID(stanza->from().bare());
	if (user==NULL)
		return true;

	if(stanza->subtype() == StanzaIqGet) {
		std::list<std::string> temp;
		temp.push_back((std::string)stanza->id());
		temp.push_back((std::string)stanza->from().full());
		vcardRequests[(std::string)stanza->to().username()]=temp;
		
		serv_get_info(purple_account_get_connection(user->account), stanza->to().username().c_str());
	}*/
/*
<iq from='romeo@montague.lit/orchard' 
    id='disco1'
    to='juliet@capulet.lit/chamber' 
    type='result'>
  <query xmlns='http://jabber.org/protocol/disco#info'
         node='http://code.google.com/p/exodus#QgayPKawpkPSDYmwT/WM94uAlu0='>
    <identity category='client' name='Exodus 0.9.1' type='pc'/>
    <feature var='http://jabber.org/protocol/caps'/>
    <feature var='http://jabber.org/protocol/disco#info'/>
    <feature var='http://jabber.org/protocol/disco#items'/>
    <feature var='http://jabber.org/protocol/muc'/>
  </query>
</iq>*/

	std::cout << "DISCO DISCO DISCO " << stanza->xml() << "\n";
	if(stanza->subtype() == StanzaIqGet && stanza->to().username()!="") {
		Tag *query = stanza->findChildWithAttrib("xmlns","http://jabber.org/protocol/disco#info");
		if (query!=NULL){
			Stanza *s = Stanza::createIqStanza(stanza->from(), stanza->id(), StanzaIqResult, "http://jabber.org/protocol/disco#info");
			s->addAttribute("node",query->findAttribute("node"));
			std::string from;
			from.append(stanza->to().full());
			s->addAttribute("from",from);
			Tag *t;
			t = new Tag("identity");
			t->addAttribute("category","gateway");
			t->addAttribute("name","High Flyer Transport");
			t->addAttribute("type",p->protocol()->gatewayIdentity());
			s->findChild("query")->addChild(t);

			std::list<std::string> features = p->protocol()->transportFeatures();
			for(std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
				t = new Tag("feature");
				t->addAttribute("var",*it);
				s->findChild("query")->addChild(t);
			}

			p->j->send( s );

			return true;
		}
	}
	return p->j->disco()->handleIq(stanza);
}

bool GlooxDiscoInfoHandler::handleIqID (Stanza *stanza, int context){
	return p->j->disco()->handleIqID(stanza,context);
}

