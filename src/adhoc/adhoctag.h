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

#ifndef _HI_ADHOC_TAG_H
#define _HI_ADHOC_TAG_H

#include <string>
#include "gloox/tag.h"

using namespace gloox;


class AdhocTag : public Tag
{
	public:
		AdhocTag(const std::string &id, const std::string &node, const std::string &status);
		~AdhocTag() {}
		void setAction(const std::string &action);
		void setTitle(const std::string &title);
		void setInstructions(const std::string &instructions);
		void addListSingle(const std::string &label, const std::string &var, std::list <std::string> &values);
		void addBoolean(const std::string &label, const std::string &var, bool value);

	private:
		void initXData();

		Tag *xdata;

};

#endif
