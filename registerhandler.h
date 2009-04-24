#ifndef _HI_CL_H
#define _HI_CL_H

#include <gloox/iqhandler.h>

class GlooxMessageHandler;
class RosterRow;



using namespace gloox;


class GlooxRegisterHandler : public IqHandler
{

public:
	GlooxRegisterHandler(GlooxMessageHandler *parent);
	~GlooxRegisterHandler();
	bool handleIq (Stanza *stanza);
	bool handleIqID (Stanza *stanza, int context);
	
	GlooxMessageHandler *p;
};

#endif
