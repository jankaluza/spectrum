#include "icq.h"
#include "../main.h"

ICQProtocol::ICQProtocol(GlooxMessageHandler *main){
	m_main = main;
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
	
	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}
	
ICQProtocol::~ICQProtocol() {}

std::string ICQProtocol::replace(std::string &str, const char *string_to_replace, const char *new_string)
{
	// Find the first string to replace
	int index = str.find(string_to_replace);
	// while there is one
	while(index != std::string::npos)
	{
		// Replace it
		str.replace(index, strlen(string_to_replace), new_string);
		// Find the next one
		index = str.find(string_to_replace, index + strlen(new_string));
	}
	return str;
}

std::string ICQProtocol::prepareUserName(std::string &str){
	str = replace(str,"-","");
	str = replace(str," ","");
	return str;
}

bool ICQProtocol::isValidUsername(std::string &str){
	std::string const digits("0123456789");
	bool allDigits( str.find_first_not_of(digits)==std::string::npos );
	return allDigits;
}

std::list<std::string> ICQProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> ICQProtocol::buddyFeatures(){
	return m_buddyFeatures;
}
