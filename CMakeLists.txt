project(spectrum)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


set(CMAKE_MODULE_PATH "cmake_modules")
include_directories(src)

set(gloox_DIR ${CMAKE_MODULE_PATH})
find_package(gloox REQUIRED)
set(purple_DIR ${CMAKE_MODULE_PATH})
find_package(purple REQUIRED)
set(glib_DIR ${CMAKE_MODULE_PATH})
find_package(glib REQUIRED)
set(libpoco_DIR ${CMAKE_MODULE_PATH})
find_package(libpoco REQUIRED)

FIND_LIBRARY(GETTEXTPO_LIBRARY NAMES gettextpo)
find_path(GETTEXTPO_INCLUDE_DIR NAMES gettext-po.h
	PATHS
	/usr/include
	/usr/local/include
)

if(GETTEXTPO_LIBRARY AND GETTEXTPO_INCLUDE_DIR)
	include (${GETTEXTPO_INCLUDE_DIR})
	message(STATUS "libgettextpo (gettext-devel) found: " ${GETTEXTPO_LIBRARY})
else(GETTEXTPO_LIBRARY AND GETTEXTPO_INCLUDE_DIR)
	message(FATAL_ERROR "no libgettextpo (gettext-devel)")
endif(GETTEXTPO_LIBRARY AND GETTEXTPO_INCLUDE_DIR)

include_directories(${GLOOX_INCLUDE_DIR})
include_directories("/usr/local/include/libpurple")
include_directories("/usr/include/libpurple")

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
	message(STATUS "Build type is set to Debug")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

#CONFIGURE_FILE(config.h.in config.h)

set(spectrum_SRCS
	src/accountcollector.cpp
	src/adhoc/adhocadmin.cpp
	src/adhoc/adhochandler.cpp
	src/adhoc/adhocrepeater.cpp
	src/adhoc/adhocsettings.cpp
	src/adhoc/dataforms.cpp
	src/autoconnectloop.cpp
	src/caps.cpp
	src/commands.cpp
	src/discoinfohandler.cpp
	src/filetransfermanager.cpp
	src/filetransferrepeater.cpp
	src/gatewayhandler.cpp
	src/geventloop.cpp
	src/localization.cpp
	src/main.cpp
	src/muchandler.cpp
	src/parser.cpp
	src/registerhandler.cpp
	src/searchhandler.cpp
	src/searchrepeater.cpp
	src/spectrum_util.cpp
	src/statshandler.cpp
	src/striphtmltags.cpp
	src/sql.cpp
	src/thread.cpp
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
	src/accountcollector.h
	src/adhoc/adhocadmin.h
	src/adhoc/adhoccommandhandler.h
	src/adhoc/adhochandler.h
	src/adhoc/adhocrepeater.h
	src/adhoc/adhocsettings.h
	src/adhoc/autoconnectloop.h
	src/adhoc/dataforms.h
	src/caps.h
	src/commands.h
	src/discoinfohandler.h
	src/filetransfermanager.h
	src/filetransferrepeater.h
	src/gatewayhandler.h
	src/geventloop.h
	src/localization.h
	src/log.h
	src/main.h
	src/muchandler.h
	src/parser.h
	src/registerhandler.h
	src/searchhandler.h
	src/searchrepeater.h
	src/spectrum_util.h
	src/statshandler.h
	src/striphtmltags.h
	src/sql.h
	src/thread.h
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

target_link_libraries(spectrum ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} ${LIBPOCO_LIBRARIES} -export-dynamic)

message(STATUS "Transport will be installed into: " ${CMAKE_INSTALL_PREFIX})
file(WRITE src/transport_config.h "#define INSTALL_DIR \"" ${CMAKE_INSTALL_PREFIX} "\"\n")

INSTALL(TARGETS spectrum
    RUNTIME DESTINATION bin
    )

INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/spectrum.cfg
	DESTINATION /etc/spectrum/
    )

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


