#ifndef ABSTRACT_TEST_H
#define ABSTRACT_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "gloox/tag.h"

using namespace gloox;

class AbstractTest : public CPPUNIT_NS :: TestFixture {
	public:
		// Sets up the test case, calls virtual up() function.
		// Called by CPPUnit.
		void setUp (void);

		// Clears all data after test, calls virtual down() function.
		// Called by CPPUnit.
		void tearDown (void);

		// Clear all tags sent by Spectrum.
		void clearTags();

		// Sets up the test case.
		virtual void up() = 0;
		
		// Clear the test case.
		virtual void down() = 0;

		// Compare two tags (or tag and string). If 'assert' is true, function
		// will raise an exception if tags are not the same.
		bool compare(Tag *received, std::string expected, bool assert = true);
		bool compare(Tag *received, Tag *expected, bool assert = true);
		bool compareTags(Tag *left, Tag *right);

		// Tries to find stanza defined as 'expected' in buffer. Raises an exception
		// if stanza is not there.
		void compare(std::string expected);

		// Get tags sent by spectrum and stores them in internal buffer.
		void getTags();

		// Tests count of tags, raises an exception if there are not 'l' tags.s
		void testTagCount(int l);

	private:
		bool _compareTags(Tag *left, Tag *right);
		std::list <Tag *> m_tags;

};

#endif
