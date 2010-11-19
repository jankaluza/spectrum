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
#include <boost/shared_ptr.hpp>

using namespace Swift;

SpectrumRegisterHandler::SpectrumRegisterHandler(Swift::Component *component) : Swift::GetResponder<Swift::InBandRegistrationPayload>(component->getIQRouter()), Swift::SetResponder<Swift::InBandRegistrationPayload>(component->getIQRouter()),
m_component(component) {

}

SpectrumRegisterHandler::~SpectrumRegisterHandler(){
}

void SpectrumRegisterHandler::start() {
	Swift::GetResponder<Swift::InBandRegistrationPayload>::start();
	Swift::SetResponder<Swift::InBandRegistrationPayload>::start();
}

bool SpectrumRegisterHandler::registerUser(const UserRow &row) {
	// TODO: move this check to sql()->addUser(...) and let it return bool
	UserRow res = Transport::instance()->sql()->getUserByJid(row.jid);
	// This user is already registered
	if (res.id != -1)
		return false;

	Log("SpectrumRegisterHandler", "adding new user: "<< row.jid << ", " << row.uin <<  ", " << row.language);
	Transport::instance()->sql()->addUser(row);

	Swift::Presence::ref response = Swift::Presence::create();
	response->setFrom(Swift::JID(Transport::instance()->jid()));
	response->setTo(Swift::JID(row.jid));
	response->setType(Swift::Presence::Subscribe);

	m_component->sendPresence(response);

	return true;
}

bool SpectrumRegisterHandler::unregisterUser(const std::string &barejid) {
	UserRow res = Transport::instance()->sql()->getUserByJid(barejid);
	// This user is already registered
	if (res.id == -1)
		return false;

	Log("SpectrumRegisterHandler", "removing user " << barejid << " from database and disconnecting from legacy network");
	PurpleAccount *account = NULL;
	Swift::Presence::ref response;

	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(barejid);
	if (user && user->hasFeature(GLOOX_FEATURE_ROSTERX) && false) {
// 		std::cout << "* sending rosterX\n";
// 		Tag *tag = new Tag("message");
// 		tag->addAttribute( "to", barejid );
// 		std::string _from;
// 		_from.append(Transport::instance()->jid());
// 		tag->addAttribute( "from", _from );
// 		tag->addChild(new Tag("body","removing users"));
// 		Tag *x = new Tag("x");
// 		x->addAttribute("xmlns","http://jabber.org/protocol/rosterx");
// 
// 		std::list <std::string> roster;
// 		roster = Transport::instance()->sql()->getBuddies(res.id);
// 
// 		Tag *item;
// 		for(std::list<std::string>::iterator u = roster.begin(); u != roster.end() ; u++){
// 			std::string name = *u;
// 			item = new Tag("item");
// 			item->addAttribute("action", "delete");
// 			item->addAttribute("jid", name + "@" + Transport::instance()->jid());
// 			x->addChild(item);
// 		}
// 
// 		tag->addChild(x);
// 		Transport::instance()->send(tag);
	}
	else {
		std::list <std::string> roster;
		// roster contains already escaped jids
		roster = Transport::instance()->sql()->getBuddies(res.id);

		for(std::list<std::string>::iterator u = roster.begin(); u != roster.end() ; u++){
			std::string name = *u;

			response = Swift::Presence::create();
			response->setTo(Swift::JID(barejid));
			response->setFrom(Swift::JID(name + "@" + Transport::instance()->jid()));
			response->setType(Swift::Presence::Unsubscribe);
			m_component->sendPresence(response);

			response = Swift::Presence::create();
			response->setTo(Swift::JID(barejid));
			response->setFrom(Swift::JID(name + "@" + Transport::instance()->jid()));
			response->setType(Swift::Presence::Unsubscribed);
			m_component->sendPresence(response);
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

	response = Swift::Presence::create();
	response->setTo(Swift::JID(barejid));
	response->setFrom(Swift::JID(Transport::instance()->jid()));
	response->setType(Swift::Presence::Unsubscribe);
	m_component->sendPresence(response);

	response = Swift::Presence::create();
	response->setTo(Swift::JID(barejid));
	response->setFrom(Swift::JID(Transport::instance()->jid()));
	response->setType(Swift::Presence::Unsubscribed);
	m_component->sendPresence(response);

	return true;
}

bool SpectrumRegisterHandler::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	if (CONFIG().protocol == "irc") {
		Swift::GetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString().getUTF8String();

	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(barejid);
	if (!CONFIG().enable_public_registration) {
		std::list<std::string> const &x = CONFIG().allowedServers;
		if (std::find(x.begin(), x.end(), from.getDomain().getUTF8String()) == x.end()) {
			Log("SpectrumRegisterHandler", "This user has no permissions to register an account");
			Swift::GetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
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
		boolean->setValue(0);
		form->addField(boolean);
	}

	reg->setForm(form);

	Swift::GetResponder<Swift::InBandRegistrationPayload>::sendResponse(from, id, reg);

	return true;
}

bool SpectrumRegisterHandler::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const Swift::String& id, boost::shared_ptr<Swift::InBandRegistrationPayload> payload) {
	if (CONFIG().protocol == "irc") {
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
		return true;
	}

	std::string barejid = from.toBare().toString().getUTF8String();

// 	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(barejid);
	if (!CONFIG().enable_public_registration) {
		std::list<std::string> const &x = CONFIG().allowedServers;
		if (std::find(x.begin(), x.end(), from.getDomain().getUTF8String()) == x.end()) {
			Log("SpectrumRegisterHandler", "This user has no permissions to register an account");
			Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::BadRequest, ErrorPayload::Modify);
			return true;
		}
	}

	UserRow res = Transport::instance()->sql()->getUserByJid(barejid);

	std::string encoding;
	std::string language;

	Form::ref form = payload->getForm();
	if (form) {
		const std::vector<FormField::ref> fields = form->getFields();
		for (std::vector<FormField::ref>::const_iterator it = fields.begin(); it != fields.end(); it++) {
			TextSingleFormField::ref textSingle = boost::dynamic_pointer_cast<TextSingleFormField>(*it);
			if (textSingle) {
				if (textSingle->getName() == "username") {
					payload->setUsername(textSingle->getValue());
				}
				else if (textSingle->getName() == "encoding") {
					encoding = textSingle->getValue().getUTF8String();
				}
				continue;
			}

			TextPrivateFormField::ref textPrivate = boost::dynamic_pointer_cast<TextPrivateFormField>(*it);
			if (textPrivate) {
				if (textPrivate->getName() == "password") {
					payload->setPassword(textPrivate->getValue());
				}
				continue;
			}

			ListSingleFormField::ref listSingle = boost::dynamic_pointer_cast<ListSingleFormField>(*it);
			if (listSingle) {
				if (listSingle->getName() == "language") {
					language = listSingle->getValue().getUTF8String();
				}
				continue;
			}

			BooleanFormField::ref boolean = boost::dynamic_pointer_cast<BooleanFormField>(*it);
			if (boolean) {
				if (boolean->getName() == "unregister") {
					if (boolean->getValue()) {
						payload->setRemove(true);
					}
				}
				continue;
			}
		}
	}

	if (payload->isRemove()) {
		unregisterUser(barejid);
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendResponse(from, id, InBandRegistrationPayload::ref());
		return true;
	}

	if (!payload->getUsername() || !payload->getPassword()) {
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

	// Register or change password
	if (payload->getUsername()->isEmpty() ||
		(payload->getPassword()->isEmpty() && CONFIG().protocol != "twitter" && CONFIG().protocol != "bonjour") ||
		localization.getLanguages().find(language) == localization.getLanguages().end())
	{
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

	if (CONFIG().protocol == "xmpp") {
		// User tries to register himself.
		if ((Swift::JID(*payload->getUsername()).toBare() == from.toBare())) {
			Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}

		// User tries to register someone who's already registered.
		UserRow user_row = Transport::instance()->sql()->getUserByJid(Swift::JID(*payload->getUsername()).toBare().toString().getUTF8String());
		if (user_row.id != -1) {
			Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
			return true;
		}
	}

	std::string username = payload->getUsername()->getUTF8String();
	Transport::instance()->protocol()->prepareUsername(username);

	std::string newUsername(username);
	if (!CONFIG().username_mask.empty()) {
		newUsername = CONFIG().username_mask;
		replace(newUsername, "$username", username.c_str());
	}

	if (!Transport::instance()->protocol()->isValidUsername(newUsername)) {
		Log("SpectrumRegisterHandler", "This is not valid username: "<< newUsername);
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}

#if GLIB_CHECK_VERSION(2,14,0)
	if (!CONFIG().reg_allowed_usernames.empty() &&
		!g_regex_match_simple(CONFIG().reg_allowed_usernames.c_str(), newUsername.c_str(),(GRegexCompileFlags) (G_REGEX_CASELESS | G_REGEX_EXTENDED), (GRegexMatchFlags) 0)) {
		Log("SpectrumRegisterHandler", "This is not valid username: "<< newUsername);
		Swift::SetResponder<Swift::InBandRegistrationPayload>::sendError(from, id, ErrorPayload::NotAcceptable, ErrorPayload::Modify);
		return true;
	}
#endif
	if (res.id == -1) {
		res.jid = barejid;
		res.uin = username;
		res.password = payload->getPassword()->getUTF8String();
		res.language = language;
		res.encoding = encoding;
		res.vip = 0;

		registerUser(res);
	}
	else {
		// change passwordhttp://soumar.jabbim.cz/phpmyadmin/index.php
		Log("SpectrumRegisterHandler", "changing user password: "<< barejid << ", " << username);
		res.jid = barejid;
		res.password = payload->getPassword()->getUTF8String();
		res.language = language;
		res.encoding = encoding;
		Transport::instance()->sql()->updateUser(res);
	}

	Swift::SetResponder<Swift::InBandRegistrationPayload>::sendResponse(from, id, InBandRegistrationPayload::ref());
	return true;
}
