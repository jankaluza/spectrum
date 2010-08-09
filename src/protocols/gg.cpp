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

#include "gg.h"

GGProtocol::GGProtocol() {
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
// 	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

GGProtocol::~GGProtocol() {}

std::list<std::string> GGProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> GGProtocol::buddyFeatures(){
	return m_buddyFeatures;
}


Tag *GGProtocol::getVCardTag(AbstractUser *user, GList *vcardEntries) {
	Tag *N = new Tag("N");
	Tag *head = new Tag("ADR");
	PurpleNotifyUserInfoEntry *vcardEntry;
	std::string firstName;
	std::string lastName;
	std::string header;
	std::string label;

	Tag *vcard = new Tag( "vCard" );
	vcard->addAttribute( "xmlns", "vcard-temp" );

	while (vcardEntries) {
		vcardEntry = (PurpleNotifyUserInfoEntry *)(vcardEntries->data);
		if (purple_notify_user_info_entry_get_label(vcardEntry) && purple_notify_user_info_entry_get_value(vcardEntry)){
			label=(std::string)purple_notify_user_info_entry_get_label(vcardEntry);
			Log("vcard label", label << " => " << (std::string)purple_notify_user_info_entry_get_value(vcardEntry));
			if (label=="First Name"){
				N->addChild( new Tag("GIVEN", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
				firstName = (std::string)purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Last Name"){
				N->addChild( new Tag("FAMILY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
				lastName = (std::string)purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Nickname"){
				vcard->addChild( new Tag("NICKNAME", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Birth Year"){
				vcard->addChild( new Tag("BDAY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="UIN"){
				vcard->addChild( new Tag("UID", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="City") {
				head = new Tag("ADR");
				head->addChild(new Tag("HOME"));
				head->addChild( new Tag("LOCALITY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
				vcard->addChild(head);
			}
		}
		vcardEntries = vcardEntries->next;
	}
	// combine first name and last name to full name
	if (!firstName.empty() || !lastName.empty())
		vcard->addChild( new Tag("FN", firstName + " " + lastName));
	// add and N if any
	if(!N->children().empty())
		vcard->addChild(N);

	return vcard;
}

std::string GGProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your GG number and password:");
	else if (key == "username")
		return _("GG number");
	return "not defined";
}

SPECTRUM_PROTOCOL(gg, GGProtocol)
