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

#include "dataforms.h"
#include <iostream>
#include <sstream>
#include <string>


Tag * xdataFromRequestInput(const std::string &title, const std::string &primaryString, const std::string &value, gboolean multiline) {
	Tag *xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	xdata->addChild(new Tag("title",title));
	xdata->addChild(new Tag("instructions",primaryString));

	Tag *field = new Tag("field");
	if (multiline)
		field->addAttribute("type","text-multi");
	else
		field->addAttribute("type","text-single");
	field->addAttribute("label","Field:");
	field->addAttribute("var","result");
	field->addChild(new Tag("value",value));
	xdata->addChild(field);

	return xdata;
}

Tag * xdataFromRequestAction(const std::string &title, const std::string &primaryString, size_t action_count, va_list acts) {
	Tag *xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	xdata->addChild(new Tag("title",title));
	xdata->addChild(new Tag("instructions",primaryString));

	Tag *field = new Tag("field");
	field->addAttribute("type","list-single");
	field->addAttribute("label","Actions");

	for (unsigned int i = 0; i < action_count; i++) {
		Tag *option;
		std::ostringstream os;
		os << i;
		std::string name(va_arg(acts, char *));
		if (i == 0)
			field->addChild(new Tag("value",os.str()));
		option = new Tag("option");
		option->addAttribute("label",name);
		option->addChild( new Tag("value",os.str()) );
		field->addChild(option);
	}
	field->addAttribute("var","result");
	xdata->addChild(field);
	
	return xdata;
}
