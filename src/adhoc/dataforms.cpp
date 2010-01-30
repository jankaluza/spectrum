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

Tag * xdataFromRequestFields(const std::string &title, const std::string &primaryString, PurpleRequestFields *fields) {
	GList *gl, *fl;
	PurpleRequestField *fld;
	Tag *field;
	Tag *xdata = new Tag("x");
	xdata->addAttribute("xmlns","jabber:x:data");
	xdata->addAttribute("type","form");
	xdata->addChild(new Tag("title",title));
	xdata->addChild(new Tag("instructions",primaryString));

	for (gl = purple_request_fields_get_groups(fields);
		 gl != NULL;
		 gl = gl->next)
	{
		GList *field_list;
		size_t field_count = 0;

		PurpleRequestFieldGroup *group = (PurpleRequestFieldGroup *) gl->data;
		field_list = purple_request_field_group_get_fields(group);

		if (purple_request_field_group_get_title(group) != NULL) {
			field = new Tag("field");
			field->addAttribute("type","fixed");
			field->addChild(new Tag("value", (std::string) purple_request_field_group_get_title(group)));
			xdata->addChild(field);
		}

		field_count = g_list_length(field_list);

		for (fl = field_list; fl != NULL; fl = fl->next){
			PurpleRequestFieldType type;
			const char *field_label;

			fld = (PurpleRequestField *) fl->data;

			if (!purple_request_field_is_visible(fld)) {
				continue;
			}

			type = purple_request_field_get_type(fld);
			field_label = purple_request_field_get_label(fld);

			if (type == PURPLE_REQUEST_FIELD_STRING) {
				const char *v = purple_request_field_string_get_default_value(fld);
				field = new Tag("field");
				if (!purple_request_field_string_is_multiline(fld))
					field->addAttribute("type","text-single");
				else
					field->addAttribute("type","text-multi");
				field->addAttribute("label", field_label ? (std::string) field_label : (std::string) purple_request_field_get_id(fld));
				field->addAttribute("var",(std::string) purple_request_field_get_id(fld));
				if (v)
					field->addChild(new Tag("value", (std::string) v));
				xdata->addChild(field);
			}
			else if (type == PURPLE_REQUEST_FIELD_INTEGER) {
				int v = purple_request_field_int_get_default_value(fld);
				field = new Tag("field");
				field->addAttribute("type","text-single");
				field->addAttribute("label", field_label ? (std::string) field_label : (std::string) purple_request_field_get_id(fld));
				field->addAttribute("var",(std::string) purple_request_field_get_id(fld));
				if (v!=0) {
					std::ostringstream os;
					os << v;
					field->addChild(new Tag("value", os.str()));
				}
				xdata->addChild(field);
			}
			else if (type == PURPLE_REQUEST_FIELD_BOOLEAN) {
				field = new Tag("field");
				field->addAttribute("type","boolean");
				field->addAttribute("label", field_label ? (std::string) field_label : (std::string) purple_request_field_get_id(fld));
				field->addAttribute("var",(std::string) purple_request_field_get_id(fld));
				if (purple_request_field_bool_get_default_value(fld))
					field->addChild(new Tag("value", "1"));
				else
					field->addChild(new Tag("value", "0"));
				xdata->addChild(field);
			}
			else if (type == PURPLE_REQUEST_FIELD_CHOICE) {
				int i = 0;
				GList *labels = purple_request_field_choice_get_labels(fld);
				GList *l;
				field = new Tag("field");
				field->addAttribute("type","list-single");
				field->addAttribute("label", field_label ? (std::string) field_label : (std::string) purple_request_field_get_id(fld));
				field->addAttribute("var",(std::string) purple_request_field_get_id(fld));

				for (l = labels; l != NULL; l = l->next, i++) {
					Tag *option;
					std::ostringstream os;
					os << i;
					std::string name((char *) l->data);
					if (i == 0)
						field->addChild(new Tag("value",os.str()));
					option = new Tag("option");
					option->addAttribute("label",name);
					option->addChild( new Tag("value",os.str()) );
					field->addChild(option);
				}
				field->addAttribute("var","result");
				xdata->addChild(field);
			}
			else if (type == PURPLE_REQUEST_FIELD_LIST) {
				int i = 0;
				GList *l;
				field = new Tag("field");
				if (purple_request_field_list_get_multi_select(fld))
					field->addAttribute("type","list-multi");
				else
					field->addAttribute("type","list-single");
				field->addAttribute("label", field_label ? (std::string) field_label : (std::string) purple_request_field_get_id(fld));
				field->addAttribute("var",(std::string) purple_request_field_get_id(fld));

				for (l = purple_request_field_list_get_items(fld); l != NULL; l = l->next, i++) {
					Tag *option;
					std::string name((char *) l->data);
					if (i == 0)
						field->addChild(new Tag("value",name));
					option = new Tag("option");
					option->addAttribute("label",name);
					option->addChild( new Tag("value",name) );
					field->addChild(option);
				}
				field->addAttribute("var","result");
				xdata->addChild(field);
			}
			else if (type == PURPLE_REQUEST_FIELD_IMAGE) {
// 				widget = create_image_field(field);
			}
			else if (type == PURPLE_REQUEST_FIELD_ACCOUNT) {
// 				widget = create_account_field(field);
			}
			else
				continue;

// 			purple_request_field_set_ui_data(field, widget);
		}
	}


	return xdata;
}

void setRequestFields(PurpleRequestFields *m_fields, Tag *x) {
	for(std::list<Tag*>::const_iterator it = x->children().begin(); it != x->children().end(); ++it) {
		std::string key = (*it)->findAttribute("var");
		if (key.empty()) continue;

		PurpleRequestField * fld = purple_request_fields_get_field(m_fields, key.c_str());
		if (!fld) continue;
		PurpleRequestFieldType type;
		type = purple_request_field_get_type(fld);

		Tag *v =(*it)->findChild("value");
		if (!v) continue;

		std::string val = v->cdata();
		if (val.empty() && type != PURPLE_REQUEST_FIELD_STRING)
			continue;

		if (type == PURPLE_REQUEST_FIELD_STRING) {
			purple_request_field_string_set_value(fld, (val.empty() ? NULL : val.c_str()));
		}
		else if (type == PURPLE_REQUEST_FIELD_INTEGER) {
			purple_request_field_int_set_value(fld, atoi(val.c_str()));
		}
		else if (type == PURPLE_REQUEST_FIELD_BOOLEAN) {
			purple_request_field_bool_set_value(fld, atoi(val.c_str()));
		}
		else if (type == PURPLE_REQUEST_FIELD_CHOICE) {
			purple_request_field_choice_set_value(fld, atoi(val.c_str()));
		}
		else if (type == PURPLE_REQUEST_FIELD_LIST) {
			for(std::list<Tag*>::const_iterator it2 = (*it)->children().begin(); it2 != (*it)->children().end(); ++it2) {
				val = (*it2)->cdata();
				if (!val.empty())
					purple_request_field_list_add_selected(fld, val.c_str());
			}
		}
	}
}
