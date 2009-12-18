#ifndef RESOURCE_MANAGER_TEST_H
#define RESOURCE_MANAGER_TEST_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "blist.h"

using namespace std;

class ResourceManager;

class ResourceManagerTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (ResourceManagerTest);
	CPPUNIT_TEST (setResource);
	CPPUNIT_TEST_SUITE_END ();

	public:
		void setUp (void);
		void tearDown (void);

	protected:
		void setResource();

	private:
		ResourceManager *m_manager;
};

CPPUNIT_TEST_SUITE_REGISTRATION (ResourceManagerTest);

#endif
