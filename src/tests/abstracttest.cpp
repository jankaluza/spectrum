#include "abstracttest.h"
#include "transport.h"
#include "../spectrum_util.h"
#include "testingbackend.h"

void AbstractTest::setUp (void) {
	up();
}

void AbstractTest::tearDown (void) {
	down();
	clearTags();
}

void AbstractTest::clearTags() {
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		m_tags.remove(tag);
		delete tag;
	}
	Transport::instance()->clearTags();
	TestingBackend *backend = (TestingBackend *) Transport::instance()->sql();
	backend->clearOnlineUsers();
}

bool AbstractTest::_compareTags(Tag *left, Tag *right) {
	if (left->name() != right->name())
		return false;
	if (left->cdata() != right->cdata())
		return false;
	
	const Tag::AttributeList &attributes = left->attributes();
	for (Tag::AttributeList::const_iterator it = attributes.begin(); it != attributes.end(); it++) {
		Tag::Attribute *attr = *it;
		if (right->findAttribute(attr->name()) != attr->value())
			return false;
	}
	
	const TagList &children_l = left->children();
	for (TagList::const_iterator it = children_l.begin(); it != children_l.end(); it++) {
		bool found = false;
		Tag *child_l = *it;
		const TagList &children_r = right->children();
		for (TagList::const_iterator it_r = children_r.begin(); it_r != children_r.end(); it_r++) {
			Tag *child_r = *it_r;
			if (_compareTags(child_l, child_r)) {
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

bool AbstractTest::compareTags(Tag *left, Tag *right) {
	return _compareTags(left, right) && _compareTags(right, left);
}

bool AbstractTest::compare(Tag *received, std::string expected, bool assert) {
	Tag *ex = Transport::instance()->parser()->getTag(expected);
	Tag *re = Transport::instance()->parser()->getTag(received->xml());
	return compare(re, ex, assert);
}

bool AbstractTest::compare(Tag *received, Tag *expected, bool assert) {
	bool tags_are_same = false;
	if (received && expected)
		tags_are_same = compareTags(received, expected);
	std::string message = "Received XML:\n";
	message += received ? received->xml() : "NULL";
	message += "\nExpected XML:\n";
	message += expected ? expected->xml() : "NULL";
	if (assert)
		CPPUNIT_ASSERT_MESSAGE(message, tags_are_same);
	return tags_are_same;
}

void AbstractTest::compare(std::string expected) {
	if (m_tags.size() == 0)
		getTags();
	for (std::list <Tag *>::iterator it = m_tags.begin(); it != m_tags.end(); it++) {
		Tag *tag = *it;
		if (compare(tag, expected, m_tags.size() == 1))
			return;
	}
	bool tag_found = false;
	std::string message("Received XML:\n");
	for (std::list <Tag *>::iterator it = m_tags.begin(); it != m_tags.end(); it++) {
		Tag *tag = *it;
		message += tag->xml() + "\n";
	}
	message += "- CANNOT FIND THIS STANZA THERE:\n";
	message += expected;
	CPPUNIT_ASSERT_MESSAGE(message, tag_found);
}

void AbstractTest::getTags() {
	m_tags = Transport::instance()->getTags();
}

void AbstractTest::testTagCount(int l) {
	if (m_tags.size() == 0)
		getTags();
	std::string message = "There has to be " + stringOf(l) + " stanza sent, but there are only " + stringOf(m_tags.size()) + ".\n";
	for (std::list <Tag *>::iterator it = m_tags.begin(); it != m_tags.end(); it++) {
		Tag *tag = *it;
		message += tag->xml() + "\n";
	}
	CPPUNIT_ASSERT_MESSAGE (message, m_tags.size() == l);
}
