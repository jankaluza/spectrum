#ifndef STRIPHTMLTAGS_H
#define STRIPHTMLTAGS_H
#include <string>

std::string& replaceAll(std::string& context,const std::string& from, const std::string& to);
std::string& stripHTMLTags(std::string& s);
#endif
