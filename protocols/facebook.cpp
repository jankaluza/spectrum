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

#include "facebook.h"
#include "../main.h"

FacebookProtocol::FacebookProtocol(GlooxMessageHandler *main){
	m_main = main;
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	m_transportFeatures.push_back("http://jabber.org/protocol/commands");
	m_transportFeatures.push_back("jabber:iq:search");
	
	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/commands");

	// load certificate from certificate.pem
	PurpleCertificatePool *tls_peers;
	tls_peers = purple_certificate_find_pool("x509", "tls_peers");
	if (!purple_certificate_pool_contains(tls_peers, "login.facebook.com")){
		PurpleCertificateScheme *x509;
		PurpleCertificate *crt;

		/* Load the scheme of our tls_peers pool (ought to be x509) */
		x509 = purple_certificate_pool_get_scheme(tls_peers);

		/* Now load the certificate from disk */
		char *c = g_build_filename(INSTALL_DIR, "share", "spectrum", "certificates", "facebook.pem", NULL);
		crt = purple_certificate_import(x509, c);
		g_free(c);
		purple_certificate_pool_store(tls_peers, "login.facebook.com", crt);

		/* And this certificate is not needed any more */
		purple_certificate_destroy(crt);
	}

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}
	
FacebookProtocol::~FacebookProtocol() {}

std::string FacebookProtocol::replace(std::string &str, const char *string_to_replace, const char *new_string)
{
	// Find the first string to replace
	int index = str.find(string_to_replace);
	// while there is one
	while(index != (int) std::string::npos)
	{
		// Replace it
		str.replace(index, strlen(string_to_replace), new_string);
		// Find the next one
		index = str.find(string_to_replace, index + strlen(new_string));
	}
	return str;
}

std::string FacebookProtocol::prepareUserName(std::string &str){
	str = replace(str," ","");
	return str;
}

bool FacebookProtocol::isValidUsername(std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> FacebookProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> FacebookProtocol::buddyFeatures(){
	return m_buddyFeatures;
}

std::string FacebookProtocol::text(const std::string &key) {
	if (key == "instructions")
		return "Enter your Facebook email and password:";
	return "not defined";
}

Tag *FacebookProtocol::getVCardTag(User *user, GList *vcardEntries) {
	PurpleNotifyUserInfoEntry *vcardEntry;
	std::string firstName;
	std::string lastName;
	std::string header;
	std::string label;
	std::string description("");

	Tag *vcard = new Tag( "vCard" );
	vcard->addAttribute( "xmlns", "vcard-temp" );

	while (vcardEntries) {
		vcardEntry = (PurpleNotifyUserInfoEntry *)(vcardEntries->data);
		if (purple_notify_user_info_entry_get_label(vcardEntry) && purple_notify_user_info_entry_get_value(vcardEntry)){
			label = (std::string) purple_notify_user_info_entry_get_label(vcardEntry);
			Log().Get("vcard label") << label << " => " << (std::string)purple_notify_user_info_entry_get_value(vcardEntry);
			if (label==tr(user->getLang(),"Gender")){
				vcard->addChild( new Tag("GENDER", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label==tr(user->getLang(),"Name")){
				vcard->addChild( new Tag("FN", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label==tr(user->getLang(),"Birthday")){
				vcard->addChild( new Tag("BDAY", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)));
			}
			else if (label==tr(user->getLang(),"Mobile")) {
				Tag *tel = new Tag("TEL");
				tel->addChild ( new Tag("CELL") );
				tel->addChild ( new Tag("NUMBER", (std::string)purple_notify_user_info_entry_get_value(vcardEntry)) );
				vcard->addChild(tel);
			}
			else {
				std::string k(purple_notify_user_info_entry_get_value(vcardEntry));
				if (!k.empty()) {
					description+= label + ": " + k + "\n\n";
				}
			}
		}
		else if (purple_notify_user_info_entry_get_type(vcardEntry)==PURPLE_NOTIFY_USER_INFO_ENTRY_SECTION_HEADER){
			header = (std::string)purple_notify_user_info_entry_get_label(vcardEntry);
			Log().Get("vcard header") << header;
// 			if (head)
// 				if (!head->children().empty())
// 					vcard->addChild(head);
// 			if (header=="Home Address"){
// 				head = new Tag("ADR");
// 				head->addChild(new Tag("HOME"));
// 			}
// 			if (header=="Work Address"){
// 				head = new Tag("ADR");
// 				head->addChild(new Tag("WORK"));
// 			}
// 			if (header=="Work Information"){
// 				head = new Tag("ORG");
// 			}
		}
		vcardEntries = vcardEntries->next;
	}
	
	// add last head if any
// 	if (head)
// 		if (!head->children().empty())
// 			vcard->addChild(head);
	vcard->addChild( new Tag("DESC", description));
	

	return vcard;
}

void FacebookProtocol::onPurpleRequestInput(User *user, const char *title, const char *primary,const char *secondary, const char *default_value,gboolean multiline, gboolean masked, gchar *hint,const char *ok_text, GCallback ok_cb,const char *cancel_text, GCallback cancel_cb, PurpleAccount *account, const char *who,PurpleConversation *conv, void *user_data) {
	if (primary){
		std::string primaryString(primary);
		if ( primaryString == "Set your Facebook status" ) {
			((PurpleRequestInputCb) ok_cb)(user_data,user->actionData.c_str());
		}
	}
}

