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

void GlooxParser::getTag(std::string str, void (*handleTagCallback)(Tag *tag, void *user_data), void *userdata) {
	m_userdata = userdata;
	m_handleTagCallback = handleTagCallback;

	m_parser->cleanup();
	if (m_parser->feed(str) != -1)
		m_handleTagCallback(NULL, m_userdata);
}

Tag *GlooxParser::getTag(std::string str) {
	m_tag = NULL;
	m_handleTagCallback = NULL;
	m_parser->cleanup();
	m_parser->feed(str);
	return m_tag;
}

void GlooxParser::handleTag(Tag *tag) {
	if (m_handleTagCallback)
		// current tag is deleted after this function, so we have to clone it.
		m_handleTagCallback(tag->clone(), m_userdata);
	else
		m_tag = tag->clone();
}


