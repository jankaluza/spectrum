#ifndef SPECTRUM_BUDDY_TEST_H
#define SPECTRUM_BUDDY_TEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../abstractspectrumbuddy.h"
#include "blist.h"

using namespace std;

class SpectrumBuddyTest : public AbstractSpectrumBuddy {
	public:
		SpectrumBuddyTest(long id, PurpleBuddy *buddy);

		std::string getAlias();
		std::string getName();
		bool getStatus(PurpleStatusPrimitive &status, std::string &statusMessage);
		std::string getIconHash();
		std::string getGroup() { return "Buddies"; }
		PurpleBuddy *getBuddy() { return NULL; }
		std::string getSafeName() { return ""; }
		
		// these functions are only in SpectrumBuddyTest class
		void setAlias(const std::string &alias);
		void setName(const std::string &name);
		void setStatus(PurpleStatusPrimitive status);
		void setStatusMessage(const std::string &statusMessage);
		void setIconHash(const std::string &iconHash);
		
	private:
		std::string m_alias;
		std::string m_name;
		std::string m_statusMessage;
		std::string m_iconHash;
		PurpleStatusPrimitive m_status;
};

#endif
