#include "gatewayhandler.h"
#include <glib.h>
#include "main.h"
#include "sql.h"

GlooxGatewayHandler::GlooxGatewayHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
}

GlooxGatewayHandler::~GlooxGatewayHandler(){
}

std::string GlooxGatewayHandler::replace(std::string &str, const char *string_to_replace, const char *new_string)
{
	// Find the first string to replace
	int index = str.find(string_to_replace);
	// while there is one
	while(index != std::string::npos)
	{
		// Replace it
		str.replace(index, strlen(string_to_replace), new_string);
		// Find the next one
		index = str.find(string_to_replace, index + strlen(new_string));
	}
	return str;
}

bool GlooxGatewayHandler::handleIq (Stanza *stanza){

	if(stanza->subtype() == StanzaIqGet) {
		//   <iq type='result' from='aim.jabber.org' to='stpeter@jabber.org/roundabout' id='gate1'>
		//     <query xmlns='jabber:iq:gateway'>
		//       <desc>
		//         Please enter the AOL Screen Name of the
		//         person you would like to contact.
		//       </desc>
		//       <prompt>Contact ID</prompt>
		//     </query>
		//   </iq>
		Stanza *s = Stanza::createIqStanza(stanza->from(), stanza->id(), StanzaIqResult, "jabber:iq:gateway");
		s->addAttribute("from",p->jid());
		
		s->findChild("query")->addChild(new Tag("desc","Please enter the ICQ Number of the person you would like to contact."));
		s->findChild("query")->addChild(new Tag("prompt","Contact ID"));
		
		p->j->send( s );
		return true;
	}
	else if(stanza->subtype() == StanzaIqSet){
		Tag *query = stanza->findChild("query");
		if (query==NULL)
			return false;
		Tag *prompt = query->findChild("prompt");
		if (prompt==NULL)
			return false;
		std::string uin(prompt->cdata());
		
		// TODO: rewrite to support more protocols
		uin = replace(uin,"-","");
		uin = replace(uin," ","");
		
		Stanza *s = Stanza::createIqStanza(stanza->from(), stanza->id(), StanzaIqResult, "jabber:iq:gateway");
		s->addAttribute("from",p->jid());
		s->findChild("query")->addChild(new Tag("jid",uin+"@"+p->jid()));
		s->findChild("query")->addChild(new Tag("prompt",uin+"@"+p->jid()));
		
		p->j->send( s );
		return true;
	}

	return false;
}

bool GlooxGatewayHandler::handleIqID (Stanza *stanza, int context){
	return false;
}
