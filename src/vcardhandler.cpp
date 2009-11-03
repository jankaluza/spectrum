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
#include "gloox/vcard.h"
#include "protocols/abstractprotocol.h"

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

static void sendVCardTag(Tag *tag, Tag *stanzaTag) {
	std::string id = stanzaTag->findAttribute("id");
	std::string from = stanzaTag->findAttribute("from");
	Tag *vcard = tag->clone();
	vcard->addAttribute("id",id);
	vcard->addAttribute("to",from);
	GlooxMessageHandler::instance()->j->send(vcard);
	delete stanzaTag;
}

GlooxVCardHandler::GlooxVCardHandler(GlooxMessageHandler *parent) : IqHandler(){
	p=parent;
	p->j->registerStanzaExtension( new VCard() );
}

GlooxVCardHandler::~GlooxVCardHandler(){
}

bool GlooxVCardHandler::handleIq (const IQ &stanza){

	if (stanza.to().username()=="")
		return false;

	User *user = p->userManager()->getUserByJID(stanza.from().bare());
	if (user==NULL)
		return false;
	if (!user->isConnected()){
		return false;
	}

	std::string name(stanza.to().username());
	std::for_each( name.begin(), name.end(), replaceJidCharacters() );
	Log("VCard", "asking for vcard" << name);
	if (stanza.subtype() == IQ::Get) {
		std::list<std::string> temp;
		temp.push_back((std::string)stanza.id());
		temp.push_back((std::string)stanza.from().full());
		vcardRequests[stanza.to().username()]=temp;
		serv_get_info(purple_account_get_connection(user->account()), name.c_str());
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

void GlooxVCardHandler::userInfoArrived(PurpleConnection *gc, const std::string &who, PurpleNotifyUserInfo *user_info){
	GList *vcardEntries = purple_notify_user_info_get_entries(user_info);
	User *user = p->userManager()->getUserByAccount(purple_connection_get_account(gc));

	if (user!=NULL){
		if (!user->isConnected())
			return;
		if (!hasVCardRequest(who))
			return;
		std::string name(who);
		std::for_each( name.begin(), name.end(), replaceJidCharacters() );

		Log("VCard", "VCard received, making vcard IQ");
		Tag *reply = new Tag( "iq" );
		reply->addAttribute( "id", vcardRequests[who].front() );
		reply->addAttribute( "type", "result" );
		reply->addAttribute( "to", vcardRequests[who].back() );
		reply->addAttribute( "from", who+"@"+p->jid() );

		Tag *vcard = p->protocol()->getVCardTag(user, vcardEntries);
		if (!vcard) {
			vcard = new Tag( "vCard" );
			vcard->addAttribute( "xmlns", "vcard-temp" );
		}

		if (purple_value_get_boolean(user->getSetting("enable_avatars")))
			Log("VCard", "AVATARS ENABLED IN USER SETTINGS");
		
		if (user->hasTransportFeature(TRANSPORT_FEATURE_AVATARS))
			Log("VCard", "TRANSPORT_FEATURE_AVATARS IS READY");

		if (purple_value_get_boolean(user->getSetting("enable_avatars")) && user->hasTransportFeature(TRANSPORT_FEATURE_AVATARS)) {
			Tag *photo = new Tag("PHOTO");

			Log("VCard", "Trying to find out " << name);
			PurpleBuddy *buddy = purple_find_buddy(purple_connection_get_account(gc), name.c_str());
			if (buddy){
				Log("VCard", "found buddy " << name);
				gsize len;
				PurpleBuddyIcon *icon = NULL;
				Log("VCard", "Is buddy_icon set? " << (purple_blist_node_get_string((PurpleBlistNode*)buddy, "buddy_icon") == NULL ? "NO" : "YES"));
				icon = purple_buddy_icons_find(purple_connection_get_account(gc), name.c_str());
				if(icon!=NULL) {
					Log("VCard", "found icon");
					const gchar * data = (gchar*)purple_buddy_icon_get_data(icon, &len);
					if (data!=NULL){
						Log("VCard", "making avatar " << len);
						std::string avatarData((char *)data,len);
						base64encode((unsigned char *)data, len, avatarData);
						photo->addChild( new Tag("BINVAL", avatarData));
						Log("VCard", "avatar made");
	// 					std::cout << photo->xml() << "\n";
					}
				}
			}

			if(!photo->children().empty())
				vcard->addChild(photo);
			else
				delete photo;
		}

		reply->addChild(vcard);
		p->j->send(reply);
		vcardRequests.erase(who);
	}
}

void GlooxVCardHandler::handleIqID (const IQ &iq, int context){
}
