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

#ifndef SPECTRUM_RESOURCE_MANAGER_H
#define SPECTRUM_RESOURCE_MANAGER_H

#include <string>
#include "glib.h"
#include <map>
#include <algorithm>
#include "gloox/presence.h"

using namespace gloox;

struct Resource {
	int priority;
	int caps;
	std::string name;
	int show;
	std::string status;
	operator bool() const {
		return !name.empty();
	}
};

// JID Resource manager Class
class ResourceManager {
	public:
		ResourceManager();
		~ResourceManager();

		// Sets resource priority and capabilities. If resource doesn't exist,
		// it's created.
		void setResource(const std::string &resource, int priority = -256, int caps = -1, int show = -1, const std::string &status = "");

		// Sets resource from Presence stanza.
		void setResource(const Presence &stanza);

		// Sets active resource. If resource is empty, resource with highest priority
		// will be set.
		void setActiveResource(const std::string &resource = "");

		// Returns true if resource already exists
		bool hasResource(const std::string &r);

		// Returns resource `r` or active resource if r is empty.
		Resource & getResource(const std::string &r = "");

		// Returns resource with particular feature.
		Resource & findResourceWithFeature(int feature);
		
		// Returns true if given resource has feature. If no resource is given,
		// then active resource is used
		bool hasFeature(int feature, const std::string &resource = "");

		int getMergedFeatures();

		// Returns all resources.
		const std::map <std::string, Resource> &getResources();

		// Removes resource.
		void removeResource(const std::string &resource);
		
	private:
		std::map <std::string, Resource> m_resources;
		std::string m_resource;

};

#endif
