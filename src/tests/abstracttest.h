#ifndef ABSTRACT_TEST_H
#define ABSTRACT_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "gloox/tag.h"

using namespace gloox;

class AbstractTest : public CPPUNIT_NS :: TestFixture {
	public:
		void setUp (void);
		void tearDown (void);
		void clearTags();
		virtual void up() = 0;
		virtual void down() = 0;
		bool compare(Tag *received, std::string expected, bool assert = true);
		bool compare(Tag *received, Tag *expected, bool assert = true);
		bool compareTags(Tag *left, Tag *right);
		void compare(std::string expected);
		void getTags();
		void testTagCount(int l);

	private:
		bool _compareTags(Tag *left, Tag *right);

		std::list <Tag *> m_tags;

};

#endif
