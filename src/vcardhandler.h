/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#ifndef _HI_VCARD_H
#define _HI_VCARD_H

#include "purple.h"
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
	bool handleIq (const IQ &iq);
	void handleIqID (const IQ &iq, int context);
	bool hasVCardRequest (const std::string &name);
	void userInfoArrived(PurpleConnection *gc, const std::string &who, PurpleNotifyUserInfo *user_info);
	GlooxMessageHandler *p;
	std::map<std::string,std::list<std::string> > vcardRequests;
};

#endif
