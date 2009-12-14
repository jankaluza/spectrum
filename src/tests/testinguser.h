#ifndef TESTING_USER_H
#define TESTING_USER_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../abstractuser.h"

using namespace std;

class TestingUser : public AbstractUser {
	public:
		TestingUser(const std::string &userKey, const std::string &jid);
		~TestingUser() { purple_value_destroy(m_value); }

		const std::string &userKey() { return m_userkey; }
		PurpleAccount *account() { return NULL; }
		const std::string &jid() { return m_jid; }
		PurpleValue *getSetting(const char *key) { return m_value; }
		bool hasTransportFeature(int feature) { return true; }
		int getFeatures() { return 256; }
		long storageId() { return 1; }
		Resource & findResourceWithFeature(int feature) { return m_resource; }
		
	private:
		PurpleValue *m_value;
		std::string m_userkey;
		std::string m_jid;
		Resource m_resource;
};

#endif
