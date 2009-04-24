#ifndef _HI_DISCOINFO_H
#define _HI_DISCOINFO_H

#include <gloox/iqhandler.h>
#include <gloox/clientbase.h>
#include <gloox/discohandler.h>
#include <glib.h>
class GlooxMessageHandler;
#include "main.h"
#include "sql.h"

using namespace gloox;


class GlooxDiscoInfoHandler : public IqHandler
{

public:
	GlooxDiscoInfoHandler(GlooxMessageHandler *parent);
	~GlooxDiscoInfoHandler();
	bool handleIq (Stanza *stanza);
	bool handleIqID (Stanza *stanza, int context);
	GlooxMessageHandler *p;

};

#endif
