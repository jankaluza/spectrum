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
#include "gloox/iq.h"

using namespace gloox;

// Tag representing adhoc dataform.
class AdhocTag : public Tag {
	public:
		// Creates new AdhocTag.
		// id     - IQ id
		// node   - node name
		// status - "executing", "executing", "completed"
		AdhocTag(const std::string &id, const std::string &node, const std::string &status);
		AdhocTag(const IQ &stanza);
		virtual ~AdhocTag() {}

		// Sets action.
		// action - "next", "complete"
		void setAction(const std::string &action);

		// Sets title.
		void setTitle(const std::string &title);

		// Sets instructions.
		void setInstructions(const std::string &instructions);

		// Adds combobox with.
		void addListSingle(const std::string &label, const std::string &var, std::list <std::string> &values);
		void addListSingle(const std::string &label, const std::string &var, std::map <std::string, std::string> &values);

		// Adds checkbox.
		void addBoolean(const std::string &label, const std::string &var, bool value);

		// Adds single line input.
		void addTextSingle(const std::string &label, const std::string &var, const std::string &value = "");
		
		// Adds single line password input.
		void addTextPrivate(const std::string &label, const std::string &var, const std::string &value = "");

		// Adds fixed text.
		void addFixedText(const std::string &text);

		// Add hidden field.
		void addHidden(const std::string &var, const std::string &value);

		// Returns value of field var.
		const std::string getValue(const std::string &var);

		// Generates response for this dataform.
		Tag * generateResponse(const std::string &action = "");

		// Returns true if XMPP user canceled executing.
		bool isCanceled();


	private:
		void initXData();
		Tag *xdata;
		std::string m_from;
		std::string m_id;
};

#endif
