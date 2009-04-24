#include "striphtmltags.h"

std::string& replaceAll(std::string& context, const std::string& from, const std::string& to) {
	size_t lookHere = 0;
	size_t foundHere;
	while((foundHere = context.find(from, lookHere)) != std::string::npos) {
		context.replace(foundHere, from.size(), to);
		lookHere = foundHere + to.size();
	}
	return context;
}

std::string& stripHTMLTags(std::string& s) {
	static bool inTag = false;
	bool done = false;

	std::string::iterator input;
	std::string::iterator output;

	input = output = s.begin();

	while (input != s.end()) {
		if (inTag) {
			inTag = !(*input++ == '>');
		} else {
			if (*input == '<') {
				inTag = true;
				input++;
			} else {
				*output++ = *input++;
			}
		}
	}
	s.erase(output, s.end());

	// Remove all special HTML characters
	replaceAll(s, "&lt;", "<");
	replaceAll(s, "&gt;", ">");
	replaceAll(s, "&amp;", "&");
	replaceAll(s, "&apos;", "'");
	replaceAll(s, "&nbsp;", " ");

    return s;
}
