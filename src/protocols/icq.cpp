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

#include "icq.h"

ICQProtocol::ICQProtocol() {
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("jabber:iq:gateway");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");

	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
	m_buddyFeatures.push_back("http://jabber.org/protocol/si");

#define XSTATUS(NAME, TYPE, XMLNS, TAG1, TAG2) \
	m_xstatus[NAME].push_back(TYPE); \
	m_xstatus[NAME].push_back(XMLNS); \
	m_xstatus[NAME].push_back(TAG1); \
	m_xstatus[NAME].push_back(TAG2);

	XSTATUS("thinking", "mood", "http://jabber.org/protocol/mood", "serious", "");
	XSTATUS("shopping", "activity", "http://jabber.org/protocol/activity", "relaxing", "shopping");
	XSTATUS("typing", "activity", "http://jabber.org/protocol/activity", "working", "typing");
	XSTATUS("question", "mood", "http://jabber.org/protocol/mood", "curious", "");
	XSTATUS("angry", "mood", "http://jabber.org/protocol/mood", "angry", "");
	XSTATUS("plate", "activity", "http://jabber.org/protocol/activity", "eating", "");
	XSTATUS("cinema", "activity", "http://jabber.org/protocol/activity", "relaxing", "watching_a_movie");
	XSTATUS("sick", "mood", "http://jabber.org/protocol/mood", "sick", "");
	XSTATUS("suit", "activity", "http://jabber.org/protocol/activity", "working", ""); // not sure
	XSTATUS("bathing", "activity", "http://jabber.org/protocol/activity", "grooming", "taking_a_bath");
	XSTATUS("tv", "activity", "http://jabber.org/protocol/activity", "relaxing", "watching_tv");
	XSTATUS("excited", "mood", "http://jabber.org/protocol/mood", "contented", "");
	XSTATUS("sleeping", "activity", "http://jabber.org/protocol/activity", "inactive", "sleeping");
// 	{"hiptop", N_("Using a PDA"), NULL},
	XSTATUS("in_love", "mood", "http://jabber.org/protocol/mood", "in_love", "");
	XSTATUS("sleepy", "mood", "http://jabber.org/protocol/mood", "sleepy", "");
	XSTATUS("meeting", "activity", "http://jabber.org/protocol/activity", "relaxing", "socializing");
	XSTATUS("phone", "activity", "http://jabber.org/protocol/activity", "talking", "on_the_phone");
	XSTATUS("surfing", "activity", "http://jabber.org/protocol/activity", "exercising", "swimming");
// 	{"mobile", N_("Mobile"), NULL},
	XSTATUS("search", "activity", "http://jabber.org/protocol/activity", "relaxing", "reading");
	XSTATUS("party", "activity", "http://jabber.org/protocol/activity", "relaxing", "partying");
	XSTATUS("coffee", "activity", "http://jabber.org/protocol/activity", "drinking", "having_a_coffee");
	XSTATUS("console", "activity", "http://jabber.org/protocol/activity", "relaxing", "gaming");
	XSTATUS("internet", "activity", "http://jabber.org/protocol/activity", "relaxing", "reading");
	XSTATUS("cigarette", "activity", "http://jabber.org/protocol/activity", "relaxing", "smoking");
	XSTATUS("writing", "activity", "http://jabber.org/protocol/activity", "working", "writing");
	XSTATUS("beer", "activity", "http://jabber.org/protocol/activity", "drinking", "having_a_beer");
	XSTATUS("music", "tune", "http://jabber.org/protocol/tune", "", "");
	XSTATUS("studying", "activity", "http://jabber.org/protocol/activity", "working", "studying");
	XSTATUS("working", "activity", "http://jabber.org/protocol/activity", "working", "");
	XSTATUS("restroom", "activity", "http://jabber.org/protocol/activity", "grooming", "");

	// probably not used ones
	XSTATUS("tired", "mood", "http://jabber.org/protocol/mood", "stressed", "");
	XSTATUS("business", "activity", "http://jabber.org/protocol/activity", "having_appointment", "");
	XSTATUS("shooting", "activity", "http://jabber.org/protocol/activity", "traveling", "commuting");
	XSTATUS("picnic", "activity", "http://jabber.org/protocol/activity", "relaxing", "going_out");
	XSTATUS("happy", "mood", "http://jabber.org/protocol/mood", "happy", "");
#undef XSTATUS
}

ICQProtocol::~ICQProtocol() {}

bool ICQProtocol::isValidUsername(const std::string &str) {
	static std::string const digits("0123456789");
	return str.find_first_not_of(digits) == std::string::npos;
}

std::list<std::string> ICQProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> ICQProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string ICQProtocol::text(const std::string &key) {
	if (key == "instructions")
		return _("Enter your UIN and password:");
	else if (key == "username")
		return _("UIN");
	return "not defined";
}

bool ICQProtocol::getXStatus(const std::string &mood, std::string &type, std::string &xmlns, std::string &tag1, std::string &tag2) {
	if (m_xstatus.find(mood) == m_xstatus.end())
		return false;
	std::vector <std::string> &l = m_xstatus[mood];
	type = l[0];
	xmlns = l[1];
	tag1 = l[2];
	tag2 = l[3];
	return true;
}

Tag *ICQProtocol::getVCardTag(User *user, GList *vcardEntries) {
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
			else if (label=="Address"){
				head->addChild( new Tag("STREET", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Zip Code"){
				head->addChild( new Tag("PCODE", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="State"){
				head->addChild( new Tag("REGION", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
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
			else if (label=="Additional Information"){
					vcard->addChild( new Tag("DESC", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label=="Email Address"){
					char *stripped = purple_markup_strip_html(purple_notify_user_info_entry_get_value(vcardEntry));
					Tag *email = new Tag("EMAIL");
					email->addChild( new Tag ("INTERNET"));
					email->addChild( new Tag ("PREF"));
					email->addChild( new Tag("USERID", stripped));
					vcard->addChild(email);
					g_free(stripped);
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

void ICQProtocol::onPurpleAccountCreated(PurpleAccount *account) {
	purple_account_set_string(account, "server", "login.icq.com");

	if (purple_version_check(2, 7, 7) != NULL) {
		purple_account_set_bool(account, "use_clientlogin", 0);
		purple_account_set_bool(account, "use_ssl", 0);
	}

}

bool ICQProtocol::onPurpleRequestInput(void *handle, User *user, const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	if (primary){
		std::string primaryString(primary);
		if (primaryString == "Authorization Request Message:") {
			((PurpleRequestInputCb) ok_cb)(user_data,tr(user->getLang(),_("Please authorize me.")));
			return true;
		}
		else if (primaryString == "Authorization Denied Message:") {
			((PurpleRequestInputCb) ok_cb)(user_data,tr(user->getLang(),_("Authorization denied.")));
			return true;
		}
	}
	return false;
}

SPECTRUM_PROTOCOL(icq, ICQProtocol)
