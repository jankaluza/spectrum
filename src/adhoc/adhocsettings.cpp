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

#include "adhocsettings.h"
#include "gloox/stanza.h"
#include "../log.h"
#include "abstractuser.h"
#include "transport.h"
#include "adhoctag.h"
#include "main.h"
#include "../usermanager.h"

AdhocSettings::AdhocSettings(AbstractUser *user, const std::string &from, const std::string &id) :
	m_from(from), m_user(user) {
	setRequestType(CALLER_ADHOC);

	if (user)
		m_language = std::string(user->getLang());
	else
		m_language = Transport::instance()->getConfiguration().language;
	
	PurpleValue *value;

	IQ _response(IQ::Result, from, id);
	Tag *response = _response.tag();
	response->addAttribute("from", Transport::instance()->jid());

	AdhocTag *adhocTag = new AdhocTag(Transport::instance()->getId(), "transport_settings", "executing");
	adhocTag->setAction("complete");
	adhocTag->setTitle(tr(m_language, _("Transport settings")));
	if (user) {
		adhocTag->setInstructions(tr(m_language, _("Change your transport settings here.")));
		
		value = m_user->getSetting("enable_transport");
		adhocTag->addBoolean(tr(m_language, _("Enable transport")), "enable_transport", purple_value_get_boolean(value));

		value = m_user->getSetting("enable_notify_email");
		adhocTag->addBoolean(tr(m_language, _("Enable notification of new email (if the legacy network supports it)")), "enable_notify_email", purple_value_get_boolean(value));

		value = m_user->getSetting("enable_avatars");
		adhocTag->addBoolean(tr(m_language, _("Enable avatars")), "enable_avatars", purple_value_get_boolean(value));

		value = m_user->getSetting("enable_chatstate");
		adhocTag->addBoolean(tr(m_language, _("Enable \"is typing\" notifications")), "enable_chatstate", purple_value_get_boolean(value));

		value = m_user->getSetting("save_files_on_server");
		adhocTag->addBoolean(tr(m_language, _("Save incoming file transfers on server")), "save_files_on_server", purple_value_get_boolean(value));
	}
	else {
		adhocTag->setInstructions(tr(m_language, _("You must be online to change transport settings.")));
	}

	response->addChild(adhocTag);
	Transport::instance()->send(response);

}

AdhocSettings::~AdhocSettings() {}

bool AdhocSettings::handleIq(const IQ &stanza) {
	Tag *stanzaTag = stanza.tag();
	Tag *tag = stanzaTag->findChild( "command" );

	if (tag->hasAttribute("action","cancel")) {
		IQ _response(IQ::Result, stanza.from().full(), stanza.id());
		_response.setFrom(Transport::instance()->jid());
		Tag *response = _response.tag();
		response->addChild( new AdhocTag(tag->findAttribute("sessionid"), "transport_settings", "canceled") );
		Transport::instance()->send(response);

		delete stanzaTag;
		return true;
	}

	m_user = Transport::instance()->userManager()->getUserByJID(stanza.from().bare());
	Tag *x = tag->findChildWithAttrib("xmlns","jabber:x:data");
	if (x && m_user) {
		std::string result("");
		for (std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it) {
			std::string key = (*it)->findAttribute("var");
			if (key.empty()) continue;

			PurpleValue * savedValue = m_user->getSetting(key.c_str());
			if (!savedValue) continue;

			Tag *v =(*it)->findChild("value");
			if (!v) continue;

			PurpleValue *value;
			if (purple_value_get_type(savedValue) == PURPLE_TYPE_BOOLEAN) {
				value = purple_value_new(PURPLE_TYPE_BOOLEAN);
				purple_value_set_boolean(value, atoi(v->cdata().c_str()));
				if (purple_value_get_boolean(savedValue) == purple_value_get_boolean(value)) {
					purple_value_destroy(value);
					continue;
				}
				m_user->updateSetting(key, value);
			}
		}

		IQ _response(IQ::Result, stanza.from().full(), stanza.id());
		_response.setFrom(Transport::instance()->jid());
		Tag *response = _response.tag();
		response->addChild( new AdhocTag(tag->findAttribute("sessionid"), "transport_settings", "completed") );
		Transport::instance()->send(response);
	}

	delete stanzaTag;
	return true;
}

