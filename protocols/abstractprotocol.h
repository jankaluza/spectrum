#ifndef _HI_ABSTRACTPROTOCOL_H
#define _HI_ABSTRACTPROTOCOL_H

#include <string>
#include <list>

class AbstractProtocol
{
	public:
		virtual ~AbstractProtocol() {}
		/*
		 * Returns gateway identity (http://xmpp.org/registrar/disco-categories.html)
		 */
		virtual std::string gatewayIdentity() = 0;
		/*
		 * Returns purple protocol name (for example "prpl-icq" for ICQ protocol)
		 */
		virtual std::string protocol() = 0;
		/*
		 * Returns true if the username is valid username for this protocol
		 */
		virtual bool isValidUsername(std::string &username) = 0;
		/*
		 * Returns revised username (for example for ICQ where username = "123- 456-789"; return "123456789";)
		 */
		virtual std::string prepareUserName(std::string &username) = 0;
		/*
		 * Returns disco features user by transport jid
		 */
		virtual std::list<std::string> transportFeatures() = 0;
		/*
		 * Returns disco features used by contacts
		 */
		virtual std::list<std::string> buddyFeatures() = 0;
};

#endif
