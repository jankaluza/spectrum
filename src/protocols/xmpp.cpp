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

#include "xmpp.h"
#include "transport.h"

XMPPProtocol::XMPPProtocol() {
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("jabber:iq:gateway");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
// 	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

XMPPProtocol::~XMPPProtocol() {}

std::list<std::string> XMPPProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> XMPPProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string XMPPProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your Jabber ID and password:");
	else if (key == "username")
		return _("Jabber ID");
	return "not defined";
}

void XMPPProtocol::makePurpleUsernameRoom(User *user, const JID &to, std::string &name) {
	std::string username = to.username();
	// "spectrum%conference.spectrum.im@irc.spectrum.im/HanzZ" -> "spectrum@conference.spectrum.im/HanzZ"
	if (!to.resource().empty()) {
		std::for_each( username.begin(), username.end(), replaceJidCharacters() );
		name.assign(username + "/" + to.resource());
	}
	// "spectrum%conference.spectrum.im@irc.spectrum.im" -> "spectrum@conference.spectrum.im"
	else {
		std::for_each( username.begin(), username.end(), replaceJidCharacters() );
		// keep old name if any
		name.assign(username + (name.empty() ? "" : ("/" + name)));
	}
}

void XMPPProtocol::makeRoomJID(User *user, std::string &name) {
	JID j(name);
	name = j.bare();
	// spectrum@conference.spectrum.im/something" -> "spectrum%conference.spectrum.im@irc.spectrum.im"
	std::transform(name.begin(), name.end(), name.begin(),(int(*)(int)) std::tolower);
	std::string name_safe = name;
	std::for_each( name_safe.begin(), name_safe.end(), replaceBadJidCharacters() ); // OK
	name.assign(name_safe + "@" + Transport::instance()->jid());
// 	if (!j.resource().empty()) {
// 		name += "/" + j.resource();
// 	}
	std::cout << "ROOMJID: " << name << "\n";
}

void XMPPProtocol::makeUsernameRoom(User *user, std::string &name) {
	// "spectrum@conferece.spectrum.im/HanzZ" -> "HanzZ"
	name = JID(name).resource();
}


void XMPPProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	std::string jid(purple_account_get_username(account));
	if (JID(jid).server() == "chat.facebook.com")
		purple_account_set_bool(account, "require_tls", false);
}

Tag *XMPPProtocol::getVCardTag(User *user, GList *vcardEntries) {
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
			if (label=="Given Name"){
				N->addChild( new Tag("GIVEN", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
				firstName = (std::string)purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Family Name"){
				N->addChild( new Tag("FAMILY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
				lastName = (std::string)purple_notify_user_info_entry_get_value(vcardEntry);
			}
			else if (label=="Nickname"){
				vcard->addChild( new Tag("NICKNAME", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Full Name"){
				vcard->addChild( new Tag("FN", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Birthday"){
				vcard->addChild( new Tag("BDAY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="City"){
				head->addChild( new Tag("LOCALITY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Country"){
				head->addChild( new Tag("CTRY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Company"){
				head->addChild( new Tag("ORGNAME", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Division"){
				head->addChild( new Tag("ORGUNIT", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Position"){
				vcard->addChild( new Tag("TITLE", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Personal Web Page"){
				std::string page = (std::string)purple_notify_user_info_entry_get_value(vcardEntry);

				//vcard->addChild( new Tag("URL", stripHTMLTags(page)));
				//vcard->addChild( new Tag("URL", page));
			}
		}
		else if (purple_notify_user_info_entry_get_type(vcardEntry)==PURPLE_NOTIFY_USER_INFO_ENTRY_SECTION_HEADER){
			header = (std::string)purple_notify_user_info_entry_get_label(vcardEntry);
			if (head)
				if (!head->children().empty())
					vcard->addChild(head);
			head = new Tag("ADR");
		}
		vcardEntries = vcardEntries->next;
	}
	// add last head if any
	if (head)
		if (!head->children().empty())
			vcard->addChild(head);
	// combine first name and last name to full name
	if ((!firstName.empty() || !lastName.empty()) && vcard->hasChild("FN"))
		vcard->addChild( new Tag("FN", firstName + " " + lastName));
	// add photo ant N if any
	if(!N->children().empty())
		vcard->addChild(N);

	return vcard;
}

SPECTRUM_PROTOCOL(xmpp, XMPPProtocol)
