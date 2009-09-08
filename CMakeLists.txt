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

if(GETTEXTPO_LIBRARY)
	message(STATUS "libgettextpo found OK")
else(GETTEXTPO_LIBRARY)
	message(FATAL_ERROR "no libgettextpo")
endif(GETTEXTPO_LIBRARY)

include_directories(${GLOOX_INCLUDE_DIR})
include_directories("/usr/local/include/libpurple")
include_directories("/usr/include/libpurple")

include_directories(${LIBPOCO_INCLUDE_DIR})

if(GLIB_FOUND)
	include_directories(${GLIB_INCLUDE_DIRS})
else(GLIB_FOUND)
	message(FATAL_ERROR "No GLIB2")
endif(GLIB_FOUND)


if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
	message(STATUS "Build type is set to Debug")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

#CONFIGURE_FILE(config.h.in config.h)

set(spectrum_SRCS
	accountcollector.cpp
	adhocadmin.cpp
	adhochandler.cpp
	adhocrepeater.cpp
	adhocsettings.cpp
	autoconnectloop.cpp
	caps.cpp
	commands.cpp
	dataforms.cpp
	discoinfohandler.cpp
	filetransfermanager.cpp
	filetransferrepeater.cpp
	gatewayhandler.cpp
	geventloop.cpp
	localization.cpp
	main.cpp
	muchandler.cpp
	parser.cpp
	registerhandler.cpp
	searchhandler.cpp
	searchrepeater.cpp
	spectrum_util.cpp
	statshandler.cpp
	striphtmltags.cpp
	sql.cpp
	thread.cpp
	user.cpp
	usermanager.cpp
	vcardhandler.cpp
	protocols/aim.cpp
	protocols/facebook.cpp
	protocols/gg.cpp
	protocols/icq.cpp
	protocols/irc.cpp
	protocols/msn.cpp
	protocols/myspace.cpp
	protocols/qq.cpp
	protocols/simple.cpp
	protocols/xmpp.cpp
	protocols/yahoo.cpp
)

set(spectrum_MOC_HDRS
	abstractpurplerequest.h
	accountcollector.h
	adhocadmin.h
	adhoccommandhandler.h
	adhochandler.h
	adhocrepeater.h
	adhocsettings.h
	autoconnectloop.h
	caps.h
	commands.h
	dataforms.h
	discoinfohandler.h
	filetransfermanager.h
	filetransferrepeater.h
	gatewayhandler.h
	geventloop.h
	localization.h
	log.h
	main.h
	muchandler.h
	parser.h
	registerhandler.h
	searchhandler.h
	searchrepeater.h
	spectrum_util.h
	statshandler.h
	striphtmltags.h
	sql.h
	thread.h
	user.h
	usermanager.h
	vcardhandler.h
	protocols/abstractprotocol.h
	protocols/aim.h
	protocols/facebook.h
	protocols/gg.h
	protocols/icq.h
	protocols/irc.h
	protocols/msn.h
	protocols/myspace.h
	protocols/qq.h
	protocols/simple.h
	protocols/xmpp.h
	protocols/yahoo.h
)

add_executable(spectrum ${spectrum_SRCS} ${lrelease_outputs})

target_link_libraries(spectrum ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${GLIB_LIBRARIES} ${GETTEXTPO_LIBRARY} ${LIBPOCO_LIBRARIES} -export-dynamic)

message(STATUS "Transport will be installed into: " ${CMAKE_INSTALL_PREFIX})
file(WRITE transport_config.h "#define INSTALL_DIR \"" ${CMAKE_INSTALL_PREFIX} "\"\n")

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


