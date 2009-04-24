#ifndef _HI_CAPS_H
#define _HI_CAPS_H

#include <gloox/iqhandler.h>
#include <gloox/discohandler.h>
class GlooxMessageHandler;

typedef enum { 	GLOOX_FEATURE_ROSTERX = 2,
				GLOOX_FEATURE_XHTML_IM = 4,
				GLOOX_FEATURE_FILETRANSFER = 8,
				GLOOX_FEATURE_CHATSTATES = 16
				} GlooxImportantFeatures;



using namespace gloox;


class GlooxDiscoHandler : public DiscoHandler
{

public:
	GlooxDiscoHandler(GlooxMessageHandler *parent);
	~GlooxDiscoHandler();
	void handleDiscoInfoResult(Stanza *stanza,int context);
	void handleDiscoItemsResult(Stanza *stanza,int context);
	void handleDiscoError(Stanza *stanza,int context);
	bool hasVersion(const std::string &name);
	GlooxMessageHandler *p;
	std::map<std::string,std::string> versions;
};

#endif
