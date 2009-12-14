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

#include "resourcemanager.h"
#include "transport.h"

Resource DummyResource;

ResourceManager::ResourceManager() {
}

ResourceManager::~ResourceManager() {
}

void ResourceManager::setResource(const std::string &resource, int priority, int caps) {
	if (!hasResource(resource))
		m_resources[resource].caps = -1;
	if (priority != -256)
		m_resources[resource].priority = priority;
	if (caps != -1)
		m_resources[resource].caps = caps;
	m_resources[resource].name = resource;
}

void ResourceManager::setResource(const Presence &stanza) {
	std::string resource = stanza.from().resource();
	m_resources[resource].priority = stanza.priority();
	Tag *stanzaTag = stanza.tag();
	Tag *c = stanzaTag->findChildWithAttrib("xmlns", "http://jabber.org/protocol/caps");
	// presence has caps
	if (c != NULL) {
		m_resources[resource].caps = Transport::instance()->getFeatures(c->findAttribute("ver"));
	}
	if (m_resources[resource].priority > m_resources[m_resource].priority)
		m_resource = resource;
	delete stanzaTag;
}

void ResourceManager::setActiveResource(const std::string &resource) {
	if (resource.empty()) {
		std::map <std::string, Resource>::iterator iter = m_resources.begin();
		m_resource = (*iter).first;
	}
	else
		m_resource = resource;
}

bool ResourceManager::hasResource(const std::string &r) {
	return m_resources.find(r) != m_resources.end();
}

Resource & ResourceManager::getResource(const std::string &r) {
	if (r.empty())
		return m_resources[m_resource];
	else
		return m_resources[r];
}

Resource & ResourceManager::findResourceWithFeature(int feature) {
	for (std::map<std::string, Resource>::iterator u = m_resources.begin(); u != m_resources.end() ; u++) {
		if ((*u).second.caps & feature)
			return getResource((*u).first);
	}
	return DummyResource;
}

bool ResourceManager::hasFeature(int feature, const std::string &resource) {
	std::string res = resource.empty() ? m_resource : resource;
	if (hasResource(res))
		return getResource(res).caps & feature;
	return false;
}

const std::map <std::string, Resource> &ResourceManager::getResources() {
	return m_resources;
}

void ResourceManager::removeResource(const std::string &resource) {
	m_resources.erase(resource);
}
