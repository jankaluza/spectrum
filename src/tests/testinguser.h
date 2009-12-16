#ifndef TESTING_USER_H
#define TESTING_USER_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../abstractuser.h"

using namespace std;

class TestingUser : public AbstractUser {
	public:
		TestingUser(const std::string &userKey, const std::string &jid);
		~TestingUser() { g_hash_table_destroy(m_settings); }

		const std::string &userKey() { return m_userkey; }
		PurpleAccount *account() { return NULL; }
		const std::string &jid() { return m_jid; }
		PurpleValue *getSetting(const char *key) { return m_value; }
		bool hasTransportFeature(int feature) { return true; }
		int getFeatures() { return 256; }
		long storageId() { return 1; }
		Resource & findResourceWithFeature(int feature) { return m_resource; }
		const std::string &username() { return m_username; }
		GHashTable *settings() { return m_settings; }
		bool isConnectedInRoom(const std::string &room) { return false; }
		bool isConnected() { return true; }

	private:
		PurpleValue *m_value;
		GHashTable *m_settings;
		std::string m_username;
		std::string m_userkey;
		std::string m_jid;
		Resource m_resource;
};

#endif
