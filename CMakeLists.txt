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
 
execute_process ( COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Python site-packages directory: " ${PYTHON_SITE_PACKAGES})

include_directories(${GLOOX_INCLUDE_DIR})
include_directories(${PURPLE_INCLUDE_DIR})

include_directories(${LIBPOCO_INCLUDE_DIR})

if(GLIB2_FOUND)
	include_directories(${GLIB2_INCLUDE_DIR})
	message( STATUS "Found GLIB2: ${GLIB2_INCLUDE_DIR} ${GLIB2_LIBRARIES}")
else(GLIB2_FOUND)
	message(FATAL_ERROR "No GLIB2")
endif(GLIB2_FOUND)


if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
	ADD_DEFINITIONS(-W)
	ADD_DEFINITIONS(-Wcast-align)
	ADD_DEFINITIONS(-Wextra -Wno-sign-compare -Wno-unused-parameter)
#	ADD_DEFINITIONS(-Werror=format-security)
	ADD_DEFINITIONS(-Winit-self)
	ADD_DEFINITIONS(-Wmissing-declarations)
#	ADD_DEFINITIONS(-Wmissing-noreturn)
	ADD_DEFINITIONS(-Wpointer-arith)
	ADD_DEFINITIONS(-Wreorder)
	ADD_DEFINITIONS(-Woverloaded-virtual)
	ADD_DEFINITIONS(-Wsign-promo)
	ADD_DEFINITIONS(-Wundef -Wunused)
#	ADD_DEFINITIONS(-Weffc++)
#	ADD_DEFINITIONS(-pedantic)
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
	src/configinterface.cpp
	src/dnsresolver.cpp
# 	src/discoinfohandler.cpp
	src/filetransfermanager.cpp
	src/filetransferrepeater.cpp
	src/gatewayhandler.cpp
	src/geventloop.cpp
	src/localization.cpp
	src/log.cpp
	src/main.cpp
	src/messagesender.cpp
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
	src/spectrumnodehandler.cpp
	src/spectrumtimer.cpp
	src/statshandler.cpp
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
	src/protocols/msn_pecan.cpp
	src/protocols/myspace.cpp
	src/protocols/qq.cpp
	src/protocols/simple.cpp
	src/protocols/sipe.cpp
	src/protocols/twitter.cpp
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
	src/configinterface.h
	src/dnsresolver.h
# 	src/discoinfohandler.h
	src/filetransfermanager.h
	src/filetransferrepeater.h
	src/gatewayhandler.h
	src/geventloop.h
	src/localization.h
	src/log.h
	src/main.h
	src/messagesender.h
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
	src/protocols/msn_pecan.h
	src/protocols/myspace.h
	src/protocols/qq.h
	src/protocols/simple.h
	src/protocols/sipe.h
	src/protocols/xmpp.h
	src/protocols/yahoo.h
)

add_executable(spectrum ${spectrum_SRCS} ${lrelease_outputs})

if(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum ${WITH_STATIC_GLOOX} -lgnutls ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${LIBPOCO_LIBRARIES} -export-dynamic)
else(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${LIBPOCO_LIBRARIES} -export-dynamic)
endif(DEFINED WITH_STATIC_GLOOX)

message(STATUS "Transport will be installed into: " ${CMAKE_INSTALL_PREFIX})
file(APPEND src/transport_config.h "#define INSTALL_DIR \"" ${CMAKE_INSTALL_PREFIX} "\"\n")

if (SPECTRUM_VERSION)
	file(APPEND src/transport_config.h "#define VERSION \"" ${SPECTRUM_VERSION} "\"\n")
else (SPECTRUM_VERSION)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
		execute_process(COMMAND git "--git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git" rev-parse --short HEAD
						OUTPUT_VARIABLE GIT_REVISION
						OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		file(APPEND src/transport_config.h "#define VERSION \"0.3-git-" ${GIT_REVISION} "\"\n")
	else (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
		file(APPEND src/transport_config.h "#define VERSION \"0.2\"\n")
	endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
endif (SPECTRUM_VERSION)












if (CPPUNIT_FOUND)

include_directories(${CPPUNIT_INCLUDE_DIR})

set(spectrum_tests_SRCS
	src/tests/abstracttest.cpp
	src/tests/autoconnectlooptest.cpp
	src/tests/capabilityhandlertest.cpp
	src/tests/configfiletest.cpp
# 	src/tests/discoinfohandlertest.cpp
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
# 	src/discoinfohandler.cpp
	src/messagesender.cpp
	src/parser.cpp
	src/rostermanager.cpp
	src/rosterstorage.cpp
	src/registerhandler.cpp
	src/resourcemanager.cpp
	src/spectrum_util.cpp
	src/spectrumconversation.cpp
	src/spectrummucconversation.cpp
	src/spectrumtimer.cpp
	src/usermanager.cpp
	src/log.cpp
	src/localization.cpp
)

set(spectrum_tests_MOC_HDRS
	src/tests/abstracttest.h
	src/tests/autoconnectlooptest.h
	src/tests/capabilityhandlertest.h
	src/tests/configfiletest.h
# 	src/tests/discoinfohandler.h
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
	src/localization.h
	src/messagesender.h
# 	src/discoinfohandler.h
	src/parser.h
	src/registerhandler.h
	src/rostermanager.h
	src/rosterstorage.h
	src/spectrum_util.h
	src/spectrumconversation.h
	src/spectrummucconversation.h
	src/spectrumtimer.h
	src/transport.h
)

add_executable(spectrum_tests ${spectrum_tests_SRCS} ${lrelease_outputs})
set_target_properties(spectrum_tests PROPERTIES COMPILE_FLAGS -DTESTS)

if(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum_tests ${WITH_STATIC_GLOOX} -lgnutls ${PURPLE_LIBRARY} ${CPPUNIT_LIBRARY} ${GLIB2_LIBRARIES} ${LIBPOCO_LIBRARIES} -export-dynamic)
else(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum_tests ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${CPPUNIT_LIBRARY} ${GLIB2_LIBRARIES} ${LIBPOCO_LIBRARIES} -export-dynamic)
endif(DEFINED WITH_STATIC_GLOOX)



endif (CPPUNIT_FOUND)


ADD_CUSTOM_TARGET (xgettext xgettext ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/protocols/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/adhoc/*.cpp --output-dir=${CMAKE_CURRENT_SOURCE_DIR}/po --from-code="UTF-8" -C --keyword="_" && sed -e 's/charset=CHARSET/charset=UTF-8/g' ${CMAKE_CURRENT_SOURCE_DIR}/po/messages.po > ${CMAKE_CURRENT_SOURCE_DIR}/po/messages.po.new && mv ${CMAKE_CURRENT_SOURCE_DIR}/po/messages.po.new ${CMAKE_CURRENT_SOURCE_DIR}/po/messages.po)

FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
 
IF(NOT GETTEXT_MSGFMT_EXECUTABLE)
	MESSAGE(
"------
                 NOTE: msgfmt not found. Translations will *not* be installed
------")
ELSE(NOT GETTEXT_MSGFMT_EXECUTABLE)
 
        SET(catalogname spectrum)
 
        ADD_CUSTOM_TARGET(translations ALL)
 
        FILE(GLOB PO_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/po/*.po)
 
        FOREACH(_poFile ${PO_FILES})
                GET_FILENAME_COMPONENT(_poFileName ${_poFile} NAME)
                STRING(REGEX REPLACE "^${catalogname}_?" "" _langCode ${_poFileName} )
                STRING(REGEX REPLACE "\\.po$" "" _langCode ${_langCode} )
				STRING(REGEX REPLACE "\\messages" "" _langCode ${_langCode} )
 
                IF( _langCode )
                        GET_FILENAME_COMPONENT(_lang po/${_poFile} NAME_WE)
                        SET(_gmoFile ${CMAKE_CURRENT_SOURCE_DIR}/po/${_lang}.mo)
 
                        ADD_CUSTOM_COMMAND(TARGET translations
                                COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check -o ${_gmoFile} ${_poFile}
                                DEPENDS ${_poFile})

                        ADD_CUSTOM_COMMAND(TARGET xgettext
                                COMMAND msgmerge ${_poFile} ${CMAKE_CURRENT_SOURCE_DIR}/po/messages.po | msgattrib --no-obsolete > ${_poFile}.new && mv ${_poFile}.new ${_poFile}
								)
                        INSTALL(FILES ${_gmoFile} DESTINATION share/locale/${_langCode}/LC_MESSAGES/ RENAME ${catalogname}.mo)
                ENDIF( _langCode )
 
        ENDFOREACH(_poFile ${PO_FILES})
 
ENDIF(NOT GETTEXT_MSGFMT_EXECUTABLE)

ADD_CUSTOM_TARGET (man ALL python ${CMAKE_CURRENT_SOURCE_DIR}/spectrumctl/make_man_page.py > ${CMAKE_CURRENT_SOURCE_DIR}/man/spectrumctl.8)


INSTALL(TARGETS spectrum
    RUNTIME DESTINATION bin
    )



INSTALL(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/spectrum.cfg
	RENAME spectrum.cfg.example
	DESTINATION share/doc/spectrum
	)

if (NOT WITHOUT_SPECTRUMCTL)

INSTALL(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/spectrumctl/spectrum
	DESTINATION ${PYTHON_SITE_PACKAGES}
    )

INSTALL(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/spectrumctl/spectrumctl.py
    DESTINATION bin
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    RENAME spectrumctl
    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ
		WORLD_READ WORLD_EXECUTE
    )

INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/man/spectrumctl.8
	DESTINATION share/man/man8
    )
INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/man/spectrum.cfg.5
	DESTINATION share/man/man5
    )

endif (NOT WITHOUT_SPECTRUMCTL)

# install(FILES
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/LICENSE.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/TRANSLATIONS.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/3RDPARTIES.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/defaults.conf
# 	DESTINATION share/fatrat/data
# 	)



