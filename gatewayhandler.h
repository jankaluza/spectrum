#ifndef _HI_GATEWAY_H
#define _HI_GATEWAY_H

#include <gloox/iqhandler.h>
#include <gloox/clientbase.h>

class GlooxMessageHandler;

using namespace gloox;


class GlooxGatewayHandler : public IqHandler
{

public:
	GlooxGatewayHandler(GlooxMessageHandler *parent);
	~GlooxGatewayHandler();
	bool handleIq (Stanza *stanza);
	bool handleIqID (Stanza *stanza, int context);
	std::string replace(std::string &str, const char *string_to_replace, const char *new_string);
	GlooxMessageHandler *p;
};

#endif
