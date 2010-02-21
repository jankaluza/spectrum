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
	CPPUNIT_ASSERT (m_manager->getResources().size() == 1);
	CPPUNIT_ASSERT (m_manager->getResource("psi").name == "psi");
	CPPUNIT_ASSERT (m_manager->getResource().name == "psi");
	CPPUNIT_ASSERT (m_manager->getResource("psi").priority == 10);
	CPPUNIT_ASSERT (m_manager->hasFeature(GLOOX_FEATURE_ROSTERX, "psi"));
	
	m_manager->setResource("gajim", 20, 0);
	CPPUNIT_ASSERT (m_manager->getResources().size() == 2);
	CPPUNIT_ASSERT (m_manager->getResource().name == "gajim");
	CPPUNIT_ASSERT (m_manager->hasFeature(GLOOX_FEATURE_ROSTERX, "gajim") == false);
	
	CPPUNIT_ASSERT (m_manager->findResourceWithFeature(GLOOX_FEATURE_ROSTERX).name == "psi");
	
	m_manager->setResource("gajim", 5, 0);
	CPPUNIT_ASSERT (m_manager->getResources().size() == 2);
	CPPUNIT_ASSERT (m_manager->getResource().name == "psi");
	
	m_manager->removeResource("psi");
	CPPUNIT_ASSERT (m_manager->getResources().size() == 1);
	CPPUNIT_ASSERT (m_manager->getResource().name == "gajim");
}
