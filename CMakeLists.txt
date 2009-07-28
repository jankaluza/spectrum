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
set(mysql_DIR ${CMAKE_MODULE_PATH})
find_package(mysql REQUIRED)

FIND_LIBRARY(GETTEXTPO_LIBRARY NAMES gettextpo)

if(GETTEXTPO_LIBRARY)
	message(STATUS "libgettextpo found OK")
else(GETTEXTPO_LIBRARY)
	message(FATAL_ERROR "no libgettextpo")
endif(GETTEXTPO_LIBRARY)

include_directories(${GLOOX_INCLUDE_DIR})
include_directories("/usr/local/include/libpurple")
include_directories("/usr/include/libpurple")

include_directories(${MySQLpp_INCLUDE_DIR})
include_directories(${MYSQL_INCLUDE_DIR})

if(GLIB_FOUND)
	include_directories(${GLIB_INCLUDE_DIRS})
else(GLIB_FOUND)
	message(FATAL_ERROR "No GLIB2")
endif(GLIB_FOUND)


if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

#CONFIGURE_FILE(config.h.in config.h)

set(spectrum_SRCS
	main.cpp
	registerhandler.cpp
	sql.cpp
	user.cpp
	xpinghandler.cpp
	statshandler.cpp
	vcardhandler.cpp
	striphtmltags.cpp
	caps.cpp
	discoinfohandler.cpp
	filetransfermanager.cpp
	sendfile.cpp
	thread.cpp
	gatewayhandler.cpp
	receivefile.cpp
	geventloop.cpp
	autoconnectloop.cpp
	usermanager.cpp
	adhochandler.cpp
	adhocrepeater.cpp
	localization.cpp
	muchandler.cpp
	adhocsettings.cpp
	adhocadmin.cpp
	searchhandler.cpp
	searchrepeater.cpp
	dataforms.cpp
	parser.cpp
	commands.cpp
	protocols/icq.cpp
	protocols/facebook.cpp
	protocols/gg.cpp
	protocols/msn.cpp
	protocols/irc.cpp
	protocols/xmpp.cpp
	protocols/myspace.cpp
	protocols/qq.cpp
	protocols/simple.cpp
	protocols/yahoo.cpp
	protocols/aim.cpp
)

set(spectrum_MOC_HDRS
	main.h
	registerhandler.h
	sql.h
	user.h
	xpinghandler.h
	statshandler.h
	vcardhandler.h
	striphtmltags.h
	caps.h
	discoinfohandler.h
	filetransfermanager.h
	sendfile.h
	thread.h
	gatewayhandler.h
	receivefile.h
	log.h
	geventloop.h
	autoconnectloop.h
	usermanager.h
	adhocrepeater.h
	adhochandler.h
	localization.h
	muchandler.h
	adhoccommandhandler.h
	adhocsettings.h
	adhocadmin.h
	searchhandler.h
	searchrepeater.h
	abstractpurplerequest.h
	dataforms.h
	parser.h
	commands.h
	protocols/abstractprotocol.h
	protocols/icq.h
	protocols/facebook.h
	protocols/gg.h
	protocols/msn.h
	protocols/irc.h
	protocols/xmpp.h
	protocols/myspace.h
	protocols/qq.h
	protocols/simple.h
	protocols/yahoo.h
	protocols/aim.h
)

add_executable(spectrum ${spectrum_SRCS} ${lrelease_outputs})

target_link_libraries(spectrum ${GLOOX_LIBRARIES} ${MySQLpp_LIBRARY} ${PURPLE_LIBRARY} ${GLIB_LIBRARIES} ${GETTEXTPO_LIBRARY} -export-dynamic)

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


