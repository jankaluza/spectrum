/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "facebook.h"
#include "../main.h"

FacebookProtocol::FacebookProtocol(GlooxMessageHandler *main){
	m_main = main;
	m_transportFeatures.push_back("jabber:iq:register");
	m_transportFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_transportFeatures.push_back("http://jabber.org/protocol/caps");
	m_transportFeatures.push_back("http://jabber.org/protocol/chatstates");
	m_transportFeatures.push_back("http://jabber.org/protocol/activity+notify");
	
	m_buddyFeatures.push_back("http://jabber.org/protocol/disco#info");
	m_buddyFeatures.push_back("http://jabber.org/protocol/caps");
	m_buddyFeatures.push_back("http://jabber.org/protocol/chatstates");

	// load certificate from certificate.pem
	PurpleCertificatePool *tls_peers;
	tls_peers = purple_certificate_find_pool("x509", "tls_peers");
	if (!purple_certificate_pool_contains(tls_peers, "login.facebook.com")){
		PurpleCertificateScheme *x509;
		PurpleCertificate *crt;

		/* Load the scheme of our tls_peers pool (ought to be x509) */
		x509 = purple_certificate_pool_get_scheme(tls_peers);

		/* Now load the certificate from disk */
		crt = purple_certificate_import(x509, "certificate.pem");
		purple_certificate_pool_store(tls_peers, "login.facebook.com", crt);

		/* And this certificate is not needed any more */
		purple_certificate_destroy(crt);
	}

// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si/profile/file-transfer");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/bytestreams");
// 	m_buddyFeatures.push_back("http://jabber.org/protocol/si");
}
	
FacebookProtocol::~FacebookProtocol() {}

std::string FacebookProtocol::replace(std::string &str, const char *string_to_replace, const char *new_string)
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

std::string FacebookProtocol::prepareUserName(std::string &str){
	str = replace(str," ","");
	return str;
}

bool FacebookProtocol::isValidUsername(std::string &str){
	// TODO: check valid email address
	return true;
}

std::list<std::string> FacebookProtocol::transportFeatures(){
	return m_transportFeatures;
}

std::list<std::string> FacebookProtocol::buddyFeatures(){
	return m_buddyFeatures;
}
