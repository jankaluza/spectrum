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

#ifndef _HI_PARSER_H
#define _HI_PARSER_H

#include <string>
#include "purple.h"
#include "gloox/parser.h"
#include "gloox/taghandler.h"

using namespace gloox;

class GlooxParser : public TagHandler
{
	public:
		GlooxParser();
		~GlooxParser();
		void getTag(std::string str, void (*handleTagCallback)(Tag *tag, Tag *user_data), Tag *userdata);

		// TagHandler
		void handleTag (Tag *tag);

	private:
		Parser *m_parser;
		void (*m_handleTagCallback)(Tag *tag, Tag *user_data);
		Tag *m_userdata;
};

#endif

