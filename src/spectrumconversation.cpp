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
#include "spectrumtimer.h"
#include "abstractspectrumbuddy.h"
#ifndef TESTS
#include "user.h"
#endif

static void sendXhtmlTag(Tag *body, void *data) {
	Tag *stanzaTag = (Tag*) data;
	if (body) {
		Tag *html = new Tag("html");
		html->addAttribute("xmlns", "http://jabber.org/protocol/xhtml-im");
		body->addAttribute("xmlns", "http://www.w3.org/1999/xhtml");
		html->addChild(body);
		stanzaTag->addChild(html);
	}
	Transport::instance()->send(stanzaTag);
}

static gboolean resendMessage(gpointer data) {
	SpectrumConversation *conv = (SpectrumConversation *) data;
	return conv->resendRawMessage();
}

SpectrumConversation::SpectrumConversation(PurpleConversation *conv, SpectrumConversationType type, const std::string &room) : AbstractConversation(type),
	m_conv(conv), m_room(room), m_resendNextRawMessage(false) {
	m_timer = new SpectrumTimer(1000, &resendMessage, this);
	m_conv->ui_data = this;
}

SpectrumConversation::~SpectrumConversation() {
	Log("SpectrumConversation", "destructor");
	delete m_timer;
}

bool SpectrumConversation::resendRawMessage() {
	for (std::map <std::string, int>::iterator it = m_rawMessages.begin(); it != m_rawMessages.end(); it++) {
		if ((*it).second == 0) {
			std::string msg((*it).first);
#ifndef TESTS
			PurpleConvIm *im = purple_conversation_get_im_data(m_conv);
			purple_conv_im_send(im, msg.c_str());
#endif
			(*it).second++;
		}
	}
	return false;
}

void SpectrumConversation::handleMessage(AbstractUser *user, const char *who, const char *msg, PurpleMessageFlags flags, time_t mtime, const std::string &currentBody) {
	std::string name(who);
	// Remove resource if it's XMPP JID
	size_t pos = name.find("/");
	if (pos != std::string::npos)
		name.erase((int) pos, name.length() - (int) pos);
	AbstractSpectrumBuddy *s_buddy = NULL;
#ifndef TESTS
	User *u = (User *) user;
	s_buddy = u->getRosterItem(name);
#endif
	if (s_buddy && s_buddy->getFlags() & SPECTRUM_BUDDY_JID_ESCAPING)
		name = JID::escapeNode(name);
	else if (s_buddy)
		std::for_each( name.begin(), name.end(), replaceBadJidCharacters() ); // OK
	else if (m_room.empty())
		name = JID::escapeNode(name);
	
	// Escape HTML characters.
	char *newline = purple_strdup_withhtml(msg);
	char *strip, *xhtml, *xhtml_linkified;
	purple_markup_html_to_xhtml(newline, &xhtml, &strip);
	xhtml_linkified = purple_markup_linkify(xhtml);
	std::string message(strip);

	std::string m(xhtml_linkified);
	if (m.find("<body>") == 0) {
		m.erase(0,6);
		m.erase(m.length() - 7, 7);
	}
	g_free(newline);
	g_free(xhtml);
	g_free(xhtml_linkified);
	g_free(strip);

	std::string to;
	if (getResource().empty())
		to = user->jid();
	else
		to = user->jid() + "/" + getResource();

	if (flags & PURPLE_MESSAGE_RAW && m_resendNextRawMessage) {
		if (m_rawMessages.find(message) == m_rawMessages.end()) {
			m_rawMessages[message] = 0;
			m_timer->start();
		}
		else if (!m_timer->isRunning()) {
			// don't send it twice in a row...
			m_rawMessages.erase(message);
		}
		m_resendNextRawMessage = false;
		return;
	}

	if (flags & PURPLE_MESSAGE_ERROR) {
		// That's handles MSN bug when switchboard timeouts and we should try to resend the message, but only just once.
// 		if (message == "Message could not be sent because an error with the switchboard occurred:") {
// 			Log("MSN SWITCHBOARD ERROR", message);
// 			m_resendNextRawMessage = true;
// 			return;
// 		} else {
			Message s(Message::Error, to, currentBody);
			if (!m_room.empty())
				s.setFrom(m_room + "%" + JID(user->username()).server() + "@" + Transport::instance()->jid() + "/" + name);
			else {
				std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
				s.setFrom(name + std::string(getType() == SPECTRUM_CONV_CHAT ? "" : ("%" + JID(user->username()).server())) + "@" + Transport::instance()->jid() + "/bot");
			}
			Error *c = new Error(StanzaErrorTypeModify, StanzaErrorNotAcceptable);
			c->setText(tr(user->getLang(), message));
			s.addExtension(c);
			Transport::instance()->send(s.tag());
			return;
// 		}
	}
	
	Message s(Message::Chat, to, message);
	if (!m_room.empty())
		s.setFrom(m_room + "@" + Transport::instance()->jid() + "/" + name);
	else {
		std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
		s.setFrom(name + std::string(getType() == SPECTRUM_CONV_CHAT ? "" : ("%" + JID(user->username()).server())) + "@" + Transport::instance()->jid() + "/bot");
	}

	// chatstates
	if (purple_value_get_boolean(user->getSetting("enable_chatstate"))) {
		if (user->hasFeature(GLOOX_FEATURE_CHATSTATES, getResource())) {
			ChatState *c = new ChatState(ChatStateActive);
			s.addExtension(c);
		}
	}

	// Delayed messages, we have to count with some delay
	if (mtime && (unsigned long) time(NULL)-10 > (unsigned long) mtime/* && (unsigned long) time(NULL) - 31536000 < (unsigned long) mtime*/) {
		char buf[80];
		strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&mtime));
		std::string timestamp(buf);
		DelayedDelivery *d = new DelayedDelivery(name + "@" + Transport::instance()->jid() + "/bot", timestamp);
		s.addExtension(d);
	}

	Tag *stanzaTag = s.tag();

	std::string res = getResource();
	if (user->hasFeature(GLOOX_FEATURE_XHTML_IM, res) && m != message) {
		Transport::instance()->parser()->getTag("<body>" + m + "</body>", sendXhtmlTag, stanzaTag);
		return;
	}
	Transport::instance()->send(stanzaTag);

}
