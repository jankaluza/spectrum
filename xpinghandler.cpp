#include "xpinghandler.h"

GlooxXPingHandler::GlooxXPingHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
}

GlooxXPingHandler::~GlooxXPingHandler(){
}

bool GlooxXPingHandler::handleIq (Stanza *stanza){
	Stanza *s = Stanza::createIqStanza(stanza->from(), stanza->id(), StanzaIqResult, "urn:xmpp:ping");

	std::string from;
	from.append(p->jid());
	s->addAttribute("from",from);
	p->j->send( s );
	return true;
}

bool GlooxXPingHandler::handleIqID (Stanza *stanza, int context){
	return false;
}
