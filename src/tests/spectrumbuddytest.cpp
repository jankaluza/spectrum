#include "spectrumbuddytest.h"

SpectrumBuddyTest::SpectrumBuddyTest(long id, PurpleBuddy *buddy) : SpectrumBuddy(id, NULL) {
}

std::string SpectrumBuddyTest::getAlias() {
	return m_alias;
}

std::string SpectrumBuddyTest::getName() {
	return m_name;
}

bool SpectrumBuddyTest::getStatus(int &status, std::string &statusMessage) {
	if (m_status == -1)
		return false;
	status = m_status;
	statusMessage = m_statusMessage;
	return true;
}

void SpectrumBuddyTest::setAlias(const std::string &alias) {
	m_alias = alias;
}

void SpectrumBuddyTest::setName(const std::string &name) {
	m_name = name;
}

void SpectrumBuddyTest::setStatus(int status) {
	m_status = status;
}

void SpectrumBuddyTest::setStatusMessage(const std::string &statusMessage) {
	m_statusMessage = statusMessage;
}


