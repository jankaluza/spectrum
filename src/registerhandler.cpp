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

#include "registerhandler.h"
#include <gloox/clientbase.h>
#include <glib.h>
#include "sql.h"
#include "capabilityhandler.h"
#include "usermanager.h"
#include "accountcollector.h"
#include "protocols/abstractprotocol.h"
#include "user.h"
#include "transport.h"
#include "main.h"
#include "Swiften/Elements/ErrorPayload.h"

using namespace Swift;

SpectrumRegisterHandler::SpectrumRegisterHandler(Swift::IQRouter *router) : Swift::GetResponder<Swift::InBandRegistrationPayload>(router) {

}

SpectrumRegisterHandler::~SpectrumRegisterHandler(){
}

bool SpectrumRegisterHandler::registerUser(const UserRow &row) {
	// TODO: move this check to sql()->addUser(...) and let it return bool
	UserRow res = Transport::instance()->sql()->getUserByJid(row.jid);
	// This user is already registered
	if (res.id != -1)
		return false;

	Log("SpectrumRegisterHandler", "adding new user: "<< row.jid << ", " << row.uin <<  ", " << row.language);
	Transport::instance()->sql()->addUser(row);

	Tag *reply = new Tag("presence");
	reply->addAttribute( "from", Transport::instance()->jid() );
	reply->addAttribute( "to", row.jid );
	reply->addAttribute( "type", "subscribe" );
	Transport::instance()->send( reply );

	return true;
}

bool SpectrumRegisterHandler::unregisterUser(const std::string &barejid) {
	UserRow res = Transport::instance()->sql()->getUserByJid(barejid);
	// This user is already registered
	if (res.id == -1)
		return false;

	Log("SpectrumRegisterHandler", "removing user " << barejid << " from database and disconnecting from legacy network");
	PurpleAccount *account = NULL;
	
	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(barejid);
	if (user && user->hasFeature(GLOOX_FEATURE_ROSTERX) && false) {
		std::cout << "* sending rosterX\n";
		Tag *tag = new Tag("message");
		tag->addAttribute( "to", barejid );
		std::string _from;
		_from.append(Transport::instance()->jid());
		tag->addAttribute( "from", _from );
		tag->addChild(new Tag("body","removing users"));
		Tag *x = new Tag("x");
		x->addAttribute("xmlns","http://jabber.org/protocol/rosterx");

		std::list <std::string> roster;
		roster = Transport::instance()->sql()->getBuddies(res.id);

		Tag *item;
		for(std::list<std::string>::iterator u = roster.begin(); u != roster.end() ; u++){
			std::string name = *u;
			item = new Tag("item");
			item->addAttribute("action", "delete");
			item->addAttribute("jid", name + "@" + Transport::instance()->jid());
			x->addChild(item);
		}

		tag->addChild(x);
		Transport::instance()->send(tag);
	}
	else {
		std::list <std::string> roster;
		// roster contains already escaped jids
		roster = Transport::instance()->sql()->getBuddies(res.id);

		Tag *tag;
		for(std::list<std::string>::iterator u = roster.begin(); u != roster.end() ; u++){
			std::string name = *u;

			tag = new Tag("presence");
			tag->addAttribute( "to", barejid );
			tag->addAttribute( "type", "unsubscribe" );
			tag->addAttribute( "from", name + "@" + Transport::instance()->jid());
			Transport::instance()->send( tag );

			tag = new Tag("presence");
			tag->addAttribute( "to", barejid );
			tag->addAttribute( "type", "unsubscribed" );
			tag->addAttribute( "from", name + "@" + Transport::instance()->jid());
			Transport::instance()->send( tag );
		}
	}

	// Remove user from database
	if (res.id != -1) {
		Transport::instance()->sql()->removeUser(res.id);
	}

	// Disconnect the user
	if (user != NULL) {
		account = user->account();
		Transport::instance()->userManager()->removeUser(user);	
	}

	if (!account) {
		account = purple_accounts_find(res.uin.c_str(), Transport::instance()->protocol()->protocol().c_str());
	}

#ifndef TESTS
	// Remove account from memory
	if (account)
		Transport::instance()->collector()->collectNow(account, true);
#endif

	Tag *reply = new Tag( "presence" );
	reply->addAttribute( "type", "unsubscribe" );
	reply->addAttribute( "from", Transport::instance()->jid() );
	reply->addAttribute( "to", barejid );
	Transport::instance()->send( reply );

	reply = new Tag("presence");
	reply->addAttribute( "type", "unsubscribed" );
	reply->addAttribute( "from", Transport::instance()->jid() );
	reply->addAttribute( "to", barejid );
	Transport::instance()->send( reply );

	return true;
}

bool SpectrumRegisterHandler::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	if (CONFIG().protocol == "irc") {
		sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString().getUTF8String();

	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(barejid);
	if (!CONFIG().enable_public_registration) {
		std::list<std::string> const &x = CONFIG().allowedServers;
		if (std::find(x.begin(), x.end(), from.getDomain().getUTF8String()) == x.end()) {
			Log("SpectrumRegisterHandler", "This user has no permissions to register an account");
			sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	const char *_language = user ? user->getLang() : CONFIG().language.c_str();

	boost::shared_ptr<InBandRegistrationPayload> reg(new InBandRegistrationPayload());

	UserRow res = Transport::instance()->sql()->getUserByJid(barejid);

	std::string instructions = CONFIG().reg_instructions.empty() ? PROTOCOL()->text("instructions") : CONFIG().reg_instructions;

	reg->setInstructions(instructions);
	reg->setRegistered(res.id != -1);
	reg->setUsername(res.uin);
	if (CONFIG().protocol != "twitter" && CONFIG().protocol != "bonjour")
		reg->setPassword(res.password);

	std::string usernameField = CONFIG().reg_username_field.empty() ? PROTOCOL()->text("username") : CONFIG().reg_username_field;

	Form::ref form(new Form(Form::FormType));
	form->setTitle(tr(_language, _("Registration")));
	form->setInstructions(tr(_language, instructions));

	HiddenFormField::ref type = HiddenFormField::create();
	type->setName("FORM_TYPE");
	type->setValue("jabber:iq:register");
	form->addField(type);

	TextSingleFormField::ref username = TextSingleFormField::create();
	username->setName("username");
	username->setLabel(tr(_language, usernameField));
	username->setValue(res.uin);
	username->setRequired(true);
	form->addField(username);

	if (CONFIG().protocol != "twitter" && CONFIG().protocol != "bonjour") {
		TextPrivateFormField::ref password = TextPrivateFormField::create();
		password->setName("password");
		password->setLabel(tr(_language, _("Password")));
		password->setRequired(true);
		form->addField(password);
	}

	ListSingleFormField::ref language = ListSingleFormField::create();
	language->setName("language");
	language->setLabel(tr(_language, _("Language")));
	if (res.id != -1)
		language->setValue(res.language);
	else
		language->setValue(CONFIG().language);
	std::map <std::string, std::string> languages = localization.getLanguages();
	for (std::map <std::string, std::string>::iterator it = languages.begin(); it != languages.end(); it++) {
		language->addOption(FormField::Option((*it).second, (*it).first));
	}
	form->addField(language);

	TextSingleFormField::ref encoding = TextSingleFormField::create();
	encoding->setName("encoding");
	encoding->setLabel(tr(_language, _("Encoding")));
	if (res.id != -1)
		encoding->setValue(res.encoding);
	else
		encoding->setValue(CONFIG().encoding);
	form->addField(encoding);

	if (res.id != -1) {
		BooleanFormField::ref boolean = BooleanFormField::create();
		boolean->setName("unregister");
		boolean->setLabel(tr(_language, _("Remove your registration")));
		form->addField(boolean);
	}

	reg->setForm(form);

	sendResponse(from, id, reg);

	return true;
}


// bool SpectrumRegisterHandler::handleIq(const Tag *iqTag) {
// 	else if (iqTag->findAttribute("type") == "set") {
// 		bool remove = false;
// 		Tag *query;
// 		Tag *usernametag;
// 		Tag *passwordtag;
// 		Tag *languagetag;
// 		Tag *encodingtag;
// 		std::string username("");
// 		std::string password("");
// 		std::string language("");
// 		std::string encoding("");
// 
// 		UserRow res = Transport::instance()->sql()->getUserByJid(from.bare());
// 		
// 		query = iqTag->findChild( "query" );
// 		if (!query) return true;
// 
// 		Tag *xdata = query->findChild("x", "xmlns", "jabber:x:data");
// 		if (xdata) {
// 			if (query->hasChild( "remove" ))
// 				remove = true;
// 			for (std::list<Tag*>::const_iterator it = xdata->children().begin(); it != xdata->children().end(); ++it) {
// 				std::string key = (*it)->findAttribute("var");
// 				if (key.empty()) continue;
// 
// 				Tag *v = (*it)->findChild("value");
// 				if (!v) continue;
// 
// 				if (key == "username")
// 					username = v->cdata();
// 				else if (key == "password")
// 					password = v->cdata();
// 				else if (key == "language")
// 					language = v->cdata();
// 				else if (key == "encoding")
// 					encoding = v->cdata();
// 				else if (key == "unregister")
// 					remove = atoi(v->cdata().c_str());
// 			}
// 		}
// 		else {
// 			if (query->hasChild( "remove" ))
// 				remove = true;
// 			else {
// 				usernametag = query->findChild("username");
// 				passwordtag = query->findChild("password");
// 				languagetag = query->findChild("language");
// 				encodingtag = query->findChild("encoding");
// 
// 				if (languagetag)
// 					language = languagetag->cdata();
// 				else
// 					language = Transport::instance()->getConfiguration().language;
// 
// 				if (encodingtag)
// 					encoding = encodingtag->cdata();
// 				else
// 					encoding = Transport::instance()->getConfiguration().encoding;
// 
// 				if (usernametag==NULL || (passwordtag==NULL && CONFIG().protocol != "twitter" && CONFIG().protocol != "bonjour")) {
// 					sendError(406, "not-acceptable", iqTag);
// 					return false;
// 				}
// 				else {
// 					username = usernametag->cdata();
// 					if (passwordtag)
// 						password = passwordtag->cdata();
// 					if (username.empty() || (password.empty() && CONFIG().protocol != "twitter" && CONFIG().protocol != "bonjour")) {
// 						sendError(406, "not-acceptable", iqTag);
// 						return false;
// 					}
// 				}
// 			}
// 		}
// 
// 		if (Transport::instance()->getConfiguration().protocol == "xmpp") {
// 			// User tries to register himself.
// 			if ((JID(username).bare() == from.bare())) {
// 				sendError(406, "not-acceptable", iqTag);
// 				return false;
// 			}
// 
// 			// User tries to register someone who's already registered.
// 			UserRow user_row = Transport::instance()->sql()->getUserByJid(JID(username).bare());
// 			if (user_row.id != -1) {
// 				sendError(406, "not-acceptable", iqTag);
// 				return false;
// 			}
// 		}
// 
// 		if (remove) {
// 			unregisterUser(from.bare());
// 
// 			Tag *reply = new Tag("iq");
// 			reply->addAttribute( "type", "result" );
// 			reply->addAttribute( "from", Transport::instance()->jid() );
// 			reply->addAttribute( "to", iqTag->findAttribute("from") );
// 			reply->addAttribute( "id", iqTag->findAttribute("id") );
// 			Transport::instance()->send( reply );
// 
// 			return true;
// 		}
// 
// 		// Register or change password
// 
// 		std::string jid = from.bare();
// 
// 		if (username.empty() || (password.empty() && CONFIG().protocol != "twitter" && CONFIG().protocol != "bonjour") || localization.getLanguages().find(language) == localization.getLanguages().end()) {
// 			sendError(406, "not-acceptable", iqTag);
// 			return false;
// 		}
// 
// 		Transport::instance()->protocol()->prepareUsername(username);
// 
// 		std::string newUsername(username);
// 		if (!CONFIG().username_mask.empty()) {
// 			newUsername = CONFIG().username_mask;
// 			replace(newUsername, "$username", username.c_str());
// 		}
// 
// 		if (!Transport::instance()->protocol()->isValidUsername(newUsername)) {
// 			Log("SpectrumRegisterHandler", "This is not valid username: "<< newUsername);
// 			sendError(400, "bad-request", iqTag);
// 			return false;
// 		}
// 
// #if GLIB_CHECK_VERSION(2,14,0)
// 		if (!CONFIG().reg_allowed_usernames.empty() &&
// 			!g_regex_match_simple(CONFIG().reg_allowed_usernames.c_str(), newUsername.c_str(),(GRegexCompileFlags) (G_REGEX_CASELESS | G_REGEX_EXTENDED), (GRegexMatchFlags) 0)) {
// 			Log("SpectrumRegisterHandler", "This is not valid username: "<< newUsername);
// 			sendError(400, "bad-request", iqTag);
// 			return false;
// 		}
// #endif
// 		if (res.id == -1) {
// 			res.jid = from.bare();
// 			res.uin = username;
// 			res.password = password;
// 			res.language = language;
// 			res.encoding = encoding;
// 			res.vip = 0;
// 
// 			registerUser(res);
// 		}
// 		else {
// 			// change passwordhttp://soumar.jabbim.cz/phpmyadmin/index.php
// 			Log("SpectrumRegisterHandler", "changing user password: "<< jid << ", " << username);
// 			res.jid = from.bare();
// 			res.password = password;
// 			res.language = language;
// 			res.encoding = encoding;
// 			Transport::instance()->sql()->updateUser(res);
// 		}
// 
// 		Tag *reply = new Tag( "iq" );
// 		reply->addAttribute( "id", iqTag->findAttribute("id") );
// 		reply->addAttribute( "type", "result" );
// 		reply->addAttribute( "to", iqTag->findAttribute("from") );
// 		reply->addAttribute( "from", Transport::instance()->jid() );
// 		Transport::instance()->send( reply );
// 
// 		return true;
// 	}
// 	return false;
// }
