project(spectrum)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

file(WRITE src/transport_config.h "\n")

set(CMAKE_MODULE_PATH "cmake_modules")
include_directories(src)


set(gloox_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(gloox REQUIRED)
set(purple_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(purple REQUIRED)
set(glib_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(glib REQUIRED)
set(libpoco_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(libpoco REQUIRED)
set(cppunit_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(cppunit)

FIND_LIBRARY(GETTEXTPO_LIBRARY NAMES gettextpo)
find_path(GETTEXTPO_INCLUDE_DIR NAMES gettext-po.h
	PATHS
	/usr/include
	/usr/local/include
)

if(GETTEXTPO_LIBRARY AND GETTEXTPO_INCLUDE_DIR)
	include_directories(${GETTEXTPO_INCLUDE_DIR})
	message(STATUS "libgettextpo (gettext-devel) found: " ${GETTEXTPO_LIBRARY})
else(GETTEXTPO_LIBRARY AND GETTEXTPO_INCLUDE_DIR)
	message(FATAL_ERROR "no libgettextpo (gettext-devel)")
endif(GETTEXTPO_LIBRARY AND GETTEXTPO_INCLUDE_DIR)

include_directories(${GLOOX_INCLUDE_DIR})
include_directories(${PURPLE_INCLUDE_DIR})

include_directories(${LIBPOCO_INCLUDE_DIR})

if(GLIB2_FOUND)
	include_directories(${GLIB2_INCLUDE_DIR})
else(GLIB2_FOUND)
	message(FATAL_ERROR "No GLIB2")
endif(GLIB2_FOUND)


if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
# 	ADD_DEFINITIONS(-pedantic)
	message(STATUS "Build type is set to Debug")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

#CONFIGURE_FILE(config.h.in config.h)

set(spectrum_SRCS
	src/accountcollector.cpp
	src/abstractspectrumbuddy.cpp
	src/abstractconversation.cpp
	src/adhoc/adhocadmin.cpp
	src/adhoc/adhochandler.cpp
	src/adhoc/adhocrepeater.cpp
	src/adhoc/adhocsettings.cpp
	src/adhoc/adhoctag.cpp
	src/adhoc/dataforms.cpp
	src/autoconnectloop.cpp
	src/capabilitymanager.cpp
	src/capabilityhandler.cpp
	src/commands.cpp
	src/configfile.cpp
	src/discoinfohandler.cpp
	src/filetransfermanager.cpp
	src/filetransferrepeater.cpp
	src/gatewayhandler.cpp
	src/geventloop.cpp
	src/localization.cpp
	src/log.cpp
	src/main.cpp
	src/spectrummessagehandler.cpp
# 	src/muchandler.cpp
	src/parser.cpp
	src/registerhandler.cpp
	src/resourcemanager.cpp
	src/rostermanager.cpp
	src/rosterstorage.cpp
	src/searchhandler.cpp
	src/searchrepeater.cpp
	src/spectrum_util.cpp
	src/spectrumbuddy.cpp
	src/spectrumconversation.cpp
	src/spectrummucconversation.cpp
	src/spectrumtimer.cpp
	src/statshandler.cpp
	src/striphtmltags.cpp
	src/sql.cpp
	src/thread.cpp
	src/transport.cpp
	src/user.cpp
	src/usermanager.cpp
	src/vcardhandler.cpp
	src/protocols/aim.cpp
	src/protocols/facebook.cpp
	src/protocols/gg.cpp
	src/protocols/icq.cpp
	src/protocols/irc.cpp
	src/protocols/msn.cpp
	src/protocols/myspace.cpp
	src/protocols/qq.cpp
	src/protocols/simple.cpp
	src/protocols/xmpp.cpp
	src/protocols/yahoo.cpp
)

set(spectrum_MOC_HDRS
	src/abstractpurplerequest.h
	src/abstractspectrumbuddy.h
	src/abstractconversation.h
	src/accountcollector.h
	src/adhoc/adhocadmin.h
	src/adhoc/adhoccommandhandler.h
	src/adhoc/adhochandler.h
	src/adhoc/adhocrepeater.h
	src/adhoc/adhocsettings.h
	src/adhoc/adhoctag.h
	src/adhoc/autoconnectloop.h
	src/adhoc/dataforms.h
	src/capabilitymanager.h
	src/capabilityhandler.h
	src/commands.h
	src/configfile.h
	src/discoinfohandler.h
	src/filetransfermanager.h
	src/filetransferrepeater.h
	src/gatewayhandler.h
	src/geventloop.h
	src/localization.h
	src/log.h
	src/main.h
	src/spectrummessagehandler.h
	src/spectrumconversation.h
# 	src/muchandler.h
	src/parser.h
	src/registerhandler.h
	src/resourcemanager.h
	src/rostermanager.h
	src/rosterstorage.h
	src/searchhandler.h
	src/searchrepeater.h
	src/spectrum_util.h
	src/spectrumbuddy.h
	src/spectrumtimer.h
	src/statshandler.h
	src/striphtmltags.h
	src/sql.h
	src/thread.h
	src/transport.h
	src/user.h
	src/usermanager.h
	src/vcardhandler.h
	src/protocols/abstractprotocol.h
	src/protocols/aim.h
	src/protocols/facebook.h
	src/protocols/gg.h
	src/protocols/icq.h
	src/protocols/irc.h
	src/protocols/msn.h
	src/protocols/myspace.h
	src/protocols/qq.h
	src/protocols/simple.h
	src/protocols/xmpp.h
	src/protocols/yahoo.h
)

add_executable(spectrum ${spectrum_SRCS} ${lrelease_outputs})

if(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum ${WITH_STATIC_GLOOX} -lgnutls ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} ${LIBPOCO_LIBRARIES} -export-dynamic)
else(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} ${LIBPOCO_LIBRARIES} -export-dynamic)
endif(DEFINED WITH_STATIC_GLOOX)

message(STATUS "Transport will be installed into: " ${CMAKE_INSTALL_PREFIX})
file(APPEND src/transport_config.h "#define INSTALL_DIR \"" ${CMAKE_INSTALL_PREFIX} "\"\n")

if (DEFINED SPECTRUM_VERSION)
	file(APPEND src/transport_config.h "#define VERSION \"" ${SPECTRUM_VERSION} "\"\n")
else (DEFINED SPECTRUM_VERSION)
	if (EXISTS .git)
		execute_process(COMMAND git rev-parse HEAD
						OUTPUT_VARIABLE GIT_REVISION
						OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		file(APPEND src/transport_config.h "#define VERSION \"0.1-git-" ${GIT_REVISION} "\"\n")
	else (EXISTS .git)
		file(APPEND src/transport_config.h "#define VERSION \"0.1\"\n")
	endif (EXISTS .git)
endif (DEFINED SPECTRUM_VERSION)












if (CPPUNIT_FOUND)

include_directories(${CPPUNIT_INCLUDE_DIR})

set(spectrum_tests_SRCS
	src/tests/autoconnectlooptest.cpp
	src/tests/capabilityhandlertest.cpp
	src/tests/configfiletest.cpp
	src/tests/discoinfohandlertest.cpp
	src/tests/main.cpp
	src/tests/spectrumbuddytest.cpp
	src/tests/spectrumconversationtest.cpp
	src/tests/spectrummucconversationtest.cpp
	src/tests/resourcemanagertest.cpp
	src/tests/registerhandlertest.cpp
	src/tests/rostermanagertest.cpp
	src/tests/rosterstoragetest.cpp
	src/tests/testingbackend.cpp
	src/tests/testingprotocol.cpp
	src/tests/transporttest.cpp
	src/tests/testinguser.cpp
	
	src/abstractconversation.cpp
	src/abstractspectrumbuddy.cpp
	src/autoconnectloop.cpp
	src/capabilityhandler.cpp
	src/capabilitymanager.cpp
	src/configfile.cpp
	src/discoinfohandler.cpp
	src/parser.cpp
	src/rostermanager.cpp
	src/rosterstorage.cpp
	src/registerhandler.cpp
	src/resourcemanager.cpp
	src/striphtmltags.cpp
	src/spectrum_util.cpp
	src/spectrumconversation.cpp
	src/spectrummucconversation.cpp
	src/spectrumtimer.cpp
	src/usermanager.cpp
	src/log.cpp
)

set(spectrum_tests_MOC_HDRS
	src/tests/autoconnectlooptest.h
	src/tests/capabilityhandlertest.h
	src/tests/configfiletest.h
	src/tests/discoinfohandler.h
	src/tests/spectrumbuddytest.h
	src/tests/spectrumconversationtest.h
	src/tests/spectrummucconversationtest.h
	src/tests/registerhandlertest.h
	src/tests/resourcemanagertest.h
	src/tests/rostermanagertest.h
	src/tests/rosterstoragetest.h
	src/tests/testingbackend.h
	src/tests/testingprotocol.h
	src/tests/testinguser.h
	
	src/abstractconversation.h
	src/abstractspectrumbuddy.h
	src/autoconnectloop.h
	src/capabilityhandler.h
	src/capabilitymanager.h
	src/configfile.h
	src/discoinfohandler.h
	src/parser.h
	src/registerhandler.h
	src/rostermanager.h
	src/rosterstorage.h
	src/striphtmltags.h
	src/spectrum_util.h
	src/spectrumconversation.h
	src/spectrummucconversation.h
	src/spectrumtimer.h
	src/transport.h
)

add_executable(spectrum_tests ${spectrum_tests_SRCS} ${lrelease_outputs})
set_target_properties(spectrum_tests PROPERTIES COMPILE_FLAGS -DTESTS)

if(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum_tests ${WITH_STATIC_GLOOX} -lgnutls ${PURPLE_LIBRARY} ${CPPUNIT_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} ${LIBPOCO_LIBRARIES} -export-dynamic)
else(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum_tests ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${CPPUNIT_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} ${LIBPOCO_LIBRARIES} -export-dynamic)
endif(DEFINED WITH_STATIC_GLOOX)



endif (CPPUNIT_FOUND)







INSTALL(TARGETS spectrum
    RUNTIME DESTINATION bin
    )


if (NOT EXISTS /etc/spectrum)
	INSTALL(FILES
		${CMAKE_CURRENT_SOURCE_DIR}/spectrum.cfg
		DESTINATION /etc/spectrum/
		)
endif (NOT EXISTS /etc/spectrum)

INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/locales/cs.po
	DESTINATION share/spectrum/locales
    )

INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/certificates/facebook.pem
	DESTINATION share/spectrum/certificates
    )

# install(FILES
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/LICENSE.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/TRANSLATIONS.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/3RDPARTIES.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/defaults.conf
# 	DESTINATION share/fatrat/data
# 	)



