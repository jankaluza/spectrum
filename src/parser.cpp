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

#include "parser.h"

GlooxParser::GlooxParser() {
	m_parser = new Parser(this);
}

GlooxParser::~GlooxParser() {
	delete m_parser;
}

void GlooxParser::getTag(std::string str, void (*handleTagCallback)(Tag *tag, Tag *user_data), Tag *userdata) {
	m_userdata = userdata;
	m_handleTagCallback = handleTagCallback;

	m_parser->cleanup();
	if (m_parser->feed(str) != -1)
		m_handleTagCallback(NULL, m_userdata);
}

void GlooxParser::handleTag(Tag *tag) {
	m_handleTagCallback(tag->clone(), m_userdata);
}


