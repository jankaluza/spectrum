#ifndef _HI_VCARD_H
#define _HI_VCARD_H


#include <gloox/iqhandler.h>
#include <gloox/clientbase.h>
#include <gloox/tag.h>
#include <gloox/base64.h>
#include <glib.h>
class GlooxMessageHandler;
#include "main.h"
#include "sql.h"
#include <sstream>
#include <fstream>
#include <iostream>

#include "account.h"
#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "eventloop.h"
#include "ft.h"
#include "log.h"
#include "notify.h"
#include "prefs.h"
#include "prpl.h"
#include "pounce.h"
#include "savedstatuses.h"
#include "sound.h"
#include "status.h"
#include "util.h"
#include "whiteboard.h"
#include "privacy.h"
#include "striphtmltags.h"

#include "user.h"

struct User;


// typedef std::list<Tag*> TagList;

using namespace gloox;


class GlooxVCardHandler : public IqHandler
{

public:
	GlooxVCardHandler(GlooxMessageHandler *parent);
	~GlooxVCardHandler();
	bool handleIq (Stanza *stanza);
	bool handleIqID (Stanza *stanza, int context);
	bool hasVCardRequest (const std::string &name);
	void userInfoArrived(PurpleConnection *gc,std::string who, PurpleNotifyUserInfo *user_info);
	GlooxMessageHandler *p;
	std::map<std::string,std::list<std::string> > vcardRequests;
};

#endif
