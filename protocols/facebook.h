#ifndef _HI_FACEBOOK_PROTOCOL_H
#define _HI_FACEBOOK_PROTOCOL_H

#include "abstractprotocol.h"

class GlooxMessageHandler;

class FacebookProtocol : AbstractProtocol
{
	public:
		FacebookProtocol(GlooxMessageHandler *main);
		~FacebookProtocol();
		std::string gatewayIdentity() { return "fb"; }
		std::string protocol() { return "prpl-bigbrownchunx-facebookim"; }
		bool isValidUsername(std::string &username);
		std::string prepareUserName(std::string &username);
		std::list<std::string> transportFeatures();
		std::list<std::string> buddyFeatures();
		
		std::string replace(std::string &str, const char *string_to_replace, const char *new_string);
	
	private:
		GlooxMessageHandler *m_main;
		std::list<std::string> m_transportFeatures;
		std::list<std::string> m_buddyFeatures;
	
};

#endif	
	