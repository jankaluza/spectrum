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

#include "vcardhandler.h"
#include "usermanager.h"
#include "log.h"

void base64encode(const unsigned char * input, int len, std::string & out)
{

    static char b64lookup[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/";

    int outlen = ((len + 2) / 3) * 4;
    unsigned long octets;
    unsigned char b2, b3, b4;
    int i, remaining;

    out.resize(outlen);

    std::string::iterator outiter = out.begin();

    for (i = 0, remaining = len; i < len;) {
        if (remaining >= 3) {
            octets = input[i] << 16;
            octets |= input[i+1] << 8;
            octets |= input[i+2];

            b4 = b64lookup[octets & 0x3F]; octets >>= 6;
            b3 = b64lookup[octets & 0x3F]; octets >>= 6;
            b2 = b64lookup[octets & 0x3F]; octets >>= 6;
            *outiter++ = b64lookup[octets & 0x3F];
            *outiter++ = b2;
            *outiter++ = b3;
            *outiter++ = b4;
            i+=3;
            remaining -= 3;
        } else if (remaining == 2) {
            octets = input[i] << 16;
            octets |= input[i+1] << 8;

            b4 = '='; octets >>= 6;
            b3 = b64lookup[octets & 0x3F]; octets >>= 6;
            b2 = b64lookup[octets & 0x3F]; octets >>= 6;
            *outiter++ = b64lookup[octets & 0x3F];
            *outiter++ = b2;
            *outiter++ = b3;
            *outiter++ = b4;
            i+=2;
            remaining -= 2; 
        } else {
            octets = input[i] << 16;

            b4 = '='; octets >>= 6;
            b3 = '='; octets >>= 6;
            b2 = b64lookup[octets & 0x3F]; octets >>= 6;
            *outiter++ = b64lookup[octets & 0x3F];
            *outiter++ = b2;
            *outiter++ = b3;
            *outiter++ = b4;
            i++;
            remaining--;  
        }
    }
}   

GlooxVCardHandler::GlooxVCardHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
}

GlooxVCardHandler::~GlooxVCardHandler(){
}

bool GlooxVCardHandler::handleIq (const IQ &stanza){

	if (stanza.from().username()=="")
		return false;

	User *user = p->userManager()->getUserByJID(stanza.from().bare());
	if (user==NULL)
		return false;
	if (!user->isConnected()){
		return false;
	}

	if(stanza.subtype() == IQ::Get) {
		std::list<std::string> temp;
		temp.push_back((std::string)stanza.id());
		temp.push_back((std::string)stanza.from().full());
		vcardRequests[(std::string)stanza.to().username()]=temp;
		
		serv_get_info(purple_account_get_connection(user->account()), stanza.to().username().c_str());
	}

	return true;
}

bool GlooxVCardHandler::hasVCardRequest(const std::string &name){
	std::map<std::string,std::list<std::string> > ::iterator iter = this->vcardRequests.begin();
	iter = this->vcardRequests.find(name);
	if(iter != this->vcardRequests.end())
		return true;
	return false;
}

void GlooxVCardHandler::userInfoArrived(PurpleConnection *gc,std::string who, PurpleNotifyUserInfo *user_info){
	GList *vcardEntries = purple_notify_user_info_get_entries(user_info);
	std::string label;
	User *user = p->userManager()->getUserByAccount(purple_connection_get_account(gc));
	PurpleNotifyUserInfoEntry *vcardEntry;

	if (user!=NULL){
		if (!user->isConnected())
			return;
		if (!hasVCardRequest(who))
			return;
		std::cout << "VCard received, making vcard IQ\n";
		Tag *reply = new Tag( "iq" );
		reply->addAttribute( "id", vcardRequests[who].front() );
		reply->addAttribute( "type", "result" );
		reply->addAttribute( "to", vcardRequests[who].back() );
		reply->addAttribute( "from", who+"@"+p->jid() );
		Tag *vcard = new Tag( "vCard" );
		vcard->addAttribute( "xmlns", "vcard-temp" );
		Tag *N = new Tag("N");
		Tag *head = new Tag("ADR");
		Tag *photo = new Tag("PHOTO");
		std::string firstName;
		std::string lastName;
		std::string header;

		PurpleBuddy *buddy = purple_find_buddy(purple_connection_get_account(gc), who.c_str());
		if (buddy){
			std::cout << "found buddy " << who << "\n";
			gsize len;
			PurpleBuddyIcon *icon = NULL;
			icon = purple_buddy_icons_find(purple_connection_get_account(gc), who.c_str());
			if(icon!=NULL) {
				std::cout << "found icon\n";
				const gchar * data = (gchar*)purple_buddy_icon_get_data(icon, &len);
				if (data!=NULL){
					std::cout << "making avatar " << len <<"\n";
					std::string avatarData((char *)data,len);
                    base64encode((unsigned char *)data, len, avatarData);
					photo->addChild( new Tag("BINVAL", avatarData));
					std::cout << "avatar made\n";
// 					std::cout << photo->xml() << "\n";
				}
			}
		}


		while (vcardEntries) {
			vcardEntry = (PurpleNotifyUserInfoEntry *)(vcardEntries->data);
			if (purple_notify_user_info_entry_get_label(vcardEntry) && purple_notify_user_info_entry_get_value(vcardEntry)){
				label=(std::string)purple_notify_user_info_entry_get_label(vcardEntry);
				Log().Get("vcard label") << label << " " << (std::string)purple_notify_user_info_entry_get_value(vcardEntry);
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
					
// 					vcard->addChild( new Tag("URL", stripHTMLTags(page)));
// 					vcard->addChild( new Tag("URL", page));
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
		if(!photo->children().empty())
			vcard->addChild(photo);

		reply->addChild(vcard);
		std::cout << reply->xml() << "\n";
		p->j->send(reply);
		vcardRequests.erase(who);
	}
}

void GlooxVCardHandler::handleIqID (const IQ &iq, int context){
}
