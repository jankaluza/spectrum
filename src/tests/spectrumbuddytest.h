#ifndef SPECTRUM_BUDDY_TEST_H
#define SPECTRUM_BUDDY_TEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../spectrumbuddy.h"
#include "blist.h"

using namespace std;

class SpectrumBuddyTest : public SpectrumBuddy {
	public:
		SpectrumBuddyTest(long id, PurpleBuddy *buddy);

		std::string getAlias();
		std::string getName();
		bool getStatus(int &status, std::string &statusMessage);
		
		// these functions are only in SpectrumBuddyTest class
		void setAlias(const std::string &alias);
		void setName(const std::string &name);
		void setStatus(int status);
		void setStatusMessage(const std::string &statusMessage);
		
	private:
		std::string m_alias;
		std::string m_name;
		std::string m_statusMessage;
		int m_status;
};

#endif
