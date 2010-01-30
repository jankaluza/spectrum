#include "testinguser.h"
#include "../capabilityhandler.h"

TestingUser::TestingUser(const std::string &userkey, const std::string &jid) {
	setResource("psi", 10, GLOOX_FEATURE_ROSTERX);
	setActiveResource("psi");
	m_value = purple_value_new(PURPLE_TYPE_BOOLEAN);
	purple_value_set_boolean(m_value, true);
	m_userkey = userkey;
	m_jid = jid;
	m_username = "user";
	m_settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) purple_value_destroy);
	g_hash_table_replace(m_settings, g_strdup("enable_avatars"), m_value);
	m_connected = true;
	m_readyForConnect = false;
	removeTimer = 0;
}


