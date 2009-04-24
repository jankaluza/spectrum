#ifndef _HI_XPING_H
#define _HI_XPING_H

#include <gloox/iqhandler.h>
#include <gloox/clientbase.h>
#include <glib.h>
class GlooxMessageHandler;
#include "main.h"
#include "sql.h"




using namespace gloox;


class GlooxXPingHandler : public IqHandler
{

public:
	GlooxXPingHandler(GlooxMessageHandler *parent);
	~GlooxXPingHandler();
	bool handleIq (Stanza *stanza);
	bool handleIqID (Stanza *stanza, int context);
	GlooxMessageHandler *p;
};

#endif
