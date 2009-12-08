#include "testinguser.h"

TestingUser::TestingUser(const std::string &userkey, const std::string &jid) {
	m_value = purple_value_new(PURPLE_TYPE_BOOLEAN);
	purple_value_set_boolean(m_value, true);
	m_userkey = userkey;
	m_jid = jid;
}
