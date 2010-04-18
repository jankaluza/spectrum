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

#ifndef _HI_DATAFORMS_H
#define _HI_DATAFORMS_H

#include <glib.h>
#include "purple.h"
#include "gloox/tag.h"
#include "request.h"

using namespace gloox;

Tag * xdataFromRequestInput(const std::string &language, const std::string &title, const std::string &primaryString, const std::string &value, gboolean multiline);
Tag * xdataFromRequestAction(const std::string &language, const std::string &title, const std::string &primaryString, size_t action_count, va_list acts);
Tag * xdataFromRequestFields(const std::string &language, const std::string &title, const std::string &primaryString, PurpleRequestFields *fields);
void setRequestFields(PurpleRequestFields *m_fields, Tag *x);

#endif
