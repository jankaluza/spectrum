#include "resourcemanagertest.h"
#include "resourcemanager.h"
#include "transport.h"
#include "testingbackend.h"
#include "../capabilityhandler.h"

void ResourceManagerTest::up (void) {
	m_manager = new ResourceManager();
}

void ResourceManagerTest::down (void) {
	delete m_manager;
}

void ResourceManagerTest::setResource() {
	m_manager->setResource("psi", 10, GLOOX_FEATURE_ROSTERX);

	// just one resource set
	CPPUNIT_ASSERT (m_manager->getResources().size() == 1);
	CPPUNIT_ASSERT (m_manager->getResource("psi").name == "psi");
	CPPUNIT_ASSERT (m_manager->getResource().name == "psi");
	CPPUNIT_ASSERT (m_manager->getResource("psi").priority == 10);
	CPPUNIT_ASSERT (m_manager->hasFeature(GLOOX_FEATURE_ROSTERX, "psi"));

	m_manager->setResource("gajim", 20, 0);

	// two resources set
	CPPUNIT_ASSERT (m_manager->getResources().size() == 2);

	// gajim has higher priority
	CPPUNIT_ASSERT (m_manager->getResource().name == "gajim");

	// gajim has no GLOOX_FEATURE_ROSTERX fetaure
	CPPUNIT_ASSERT (m_manager->hasFeature(GLOOX_FEATURE_ROSTERX, "gajim") == false);

	// psi has GLOOX_FEATURE_ROSTERX fetaure
	CPPUNIT_ASSERT (m_manager->findResourceWithFeature(GLOOX_FEATURE_ROSTERX).name == "psi");

	// change priority to 5, so psi becomes highest resource
	m_manager->setResource("gajim", 5, 0);
	CPPUNIT_ASSERT (m_manager->getResources().size() == 2);
	CPPUNIT_ASSERT (m_manager->getResource().name == "psi");

	// remove psi, so there's only one resource and that's gajim.
	m_manager->removeResource("psi");
	CPPUNIT_ASSERT (m_manager->getResources().size() == 1);
	CPPUNIT_ASSERT (m_manager->getResource().name == "gajim");
}

void ResourceManagerTest::defaultValues() {
	// empty resources are not allowed
	m_manager->setResource("");
	CPPUNIT_ASSERT (m_manager->getResources().size() == 0);
	
	// test if default values are set
	m_manager->setResource("psi");
	CPPUNIT_ASSERT (m_manager->getResources().size() == 1);
	CPPUNIT_ASSERT (m_manager->getResource("psi").name == "psi");
	CPPUNIT_ASSERT (m_manager->getResource("psi").priority == 0);
	CPPUNIT_ASSERT (m_manager->getResource("psi").caps == 0);
}
