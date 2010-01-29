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

#include "spectrumconversation.h"
#include "main.h" // just for replaceBadJidCharacters
#include "capabilityhandler.h"
#include "gloox/chatstate.h"
#include "gloox/error.h"
#include "transport.h"
#include "usermanager.h"

static void sendXhtmlTag(Tag *body, Tag *stanzaTag) {
	if (body) {
		Tag *html = new Tag("html");
		html->addAttribute("xmlns", "http://jabber.org/protocol/xhtml-im");
		body->addAttribute("xmlns", "http://www.w3.org/1999/xhtml");
		html->addChild(body);
		stanzaTag->addChild(html);
	}
	Transport::instance()->send(stanzaTag);
}

SpectrumConversation::SpectrumConversation(PurpleConversation *conv, SpectrumConversationType type) : AbstractConversation(type),
	m_conv(conv) {
}

SpectrumConversation::~SpectrumConversation() {
}

void SpectrumConversation::handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime, const std::string &currentBody) {

	std::string name(who);
	// Remove resource if it's XMPP JID
	size_t pos = name.find("/");
	if (pos != std::string::npos)
		name.erase((int) pos, name.length() - (int) pos);

	std::for_each( name.begin(), name.end(), replaceBadJidCharacters() );
	std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
	
	// Escape HTML characters.
	char *newline = purple_strdup_withhtml(msg);
	char *strip, *xhtml;
	purple_markup_html_to_xhtml(newline, &xhtml, &strip);
	std::string message(strip);

	std::string to;
	if (getResource().empty())
		to = user->jid();
	else
		to = user->jid() + "/" + getResource();

	if (flags & PURPLE_MESSAGE_ERROR /* && message == "Unable to send message: The message is too large."*/) {
		Message s(Message::Error, to, currentBody);
		s.setFrom(name + std::string(getType() == SPECTRUM_CONV_CHAT ? "" : ("%" + JID(user->username()).server())) + "@" + Transport::instance()->jid() + "/bot");
		Error *c = new Error(StanzaErrorTypeModify, StanzaErrorNotAcceptable);
		c->setText(message);
		s.addExtension(c);
		Transport::instance()->send(s.tag());
		g_free(newline);
		g_free(xhtml);
		g_free(strip);
		return;
	}
	
	
	Message s(Message::Chat, to, message);
	s.setFrom(name + std::string(getType() == SPECTRUM_CONV_CHAT ? "" : ("%" + JID(user->username()).server())) + "@" + Transport::instance()->jid() + "/bot");

	// chatstates
	if (purple_value_get_boolean(user->getSetting("enable_chatstate"))) {
		if (user->hasFeature(GLOOX_FEATURE_CHATSTATES, getResource())) {
			ChatState *c = new ChatState(ChatStateActive);
			s.addExtension(c);
		}
	}

	// Delayed messages, we have to count with some delay
	if ((unsigned long) time(NULL)-10 > (unsigned long) mtime && (unsigned long) time(NULL) - 31536000 > (unsigned long) mtime) {
		char buf[80];
		strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&mtime));
		std::string timestamp(buf);
		DelayedDelivery *d = new DelayedDelivery(name + "@" + Transport::instance()->jid() + "/bot", timestamp);
		s.addExtension(d);
	}

	Tag *stanzaTag = s.tag();

	std::string m(msg);
	if (m.find("<body>") == 0) {
		m.erase(0,6);
		m.erase(m.length() - 7, 7);
	}
	g_free(newline);
	g_free(xhtml);
	g_free(strip);

	std::string res = getResource();
	if (user->hasFeature(GLOOX_FEATURE_XHTML_IM, res) && m != message) {
		Transport::instance()->parser()->getTag("<body>" + m + "</body>", sendXhtmlTag, stanzaTag);
		return;
	}
	Transport::instance()->send(stanzaTag);

}
