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

#include "msn_pecan.h"
#include "../main.h"

MSNPecanProtocol::MSNPecanProtocol(GlooxMessageHandler *main){
	m_main = main;
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

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}

MSNPecanProtocol::~MSNPecanProtocol() {}

bool MSNPecanProtocol::isValidUsername(const std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> MSNPecanProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> MSNPecanProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string MSNPecanProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your Passport username and pssword:");
	return "not defined";
}

Tag *MSNPecanProtocol::getVCardTag(AbstractUser *user, GList *vcardEntries) {
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
			else if (label=="Gender"){
				vcard->addChild( new Tag("GENDER", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Nick"){
				vcard->addChild( new Tag("NICKNAME", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Birthday"){
				vcard->addChild( new Tag("BDAY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="UIN"){
				vcard->addChild( new Tag("UID", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="City"){
				head->addChild( new Tag("LOCALITY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="State"){
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
			if (header=="Home Address"){
				head = new Tag("ADR");
				head->addChild(new Tag("HOME"));
			}
			if (header=="Work Address"){
				head = new Tag("ADR");
				head->addChild(new Tag("WORK"));
			}
			if (header=="Work Information"){
				head = new Tag("ORG");
			}
		}
		vcardEntries = vcardEntries->next;
	}
	// add last head if any
	if (head)
		if (!head->children().empty())
			vcard->addChild(head);
	// combine first name and last name to full name
	if (!firstName.empty() || !lastName.empty())
		vcard->addChild( new Tag("FN", firstName + " " + lastName));
	// add photo ant N if any
	if(!N->children().empty())
		vcard->addChild(N);

	return vcard;
}

bool MSNPecanProtocol::onPurpleRequestInput(AbstractUser *user, const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	if (primary){
		std::string primaryString(primary);
		if (primaryString == "Authorization Request Message:") {
			((PurpleRequestInputCb) ok_cb)(user_data,tr(user->getLang(),_("Please authorize me.")));
			return true;
		}
	}
	return false;
}

