project(hiicq)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


set(CMAKE_MODULE_PATH cmake_modules)
include_directories(src)

ADD_CUSTOM_TARGET(uninstall
	COMMAND ${CMAKE_COMMAND} -E echo Use 'xargs rm < install_manifest.txt' to uninstall this program
	)

# if(WITH_EVERYTHING)
# 
# 	set(WITH_JABBER ON)
# endif(WITH_EVERYTHING)

set(gloox_DIR ${CMAKE_MODULE_PATH})
find_package(gloox REQUIRED)
set(purple_DIR ${CMAKE_MODULE_PATH})
find_package(purple REQUIRED)
set(glib_DIR ${CMAKE_MODULE_PATH})
find_package(glib REQUIRED)

if(GLOOX_FOUND)
	message(STATUS "gloox found OK")
	include_directories(${GLOOX_INCLUDE_DIRS})
else(GLOOX_FOUND)
	message(FATAL_ERROR "No gloox")
endif(GLOOX_FOUND)

include_directories("/usr/include/mysql++")
include_directories("/usr/include/mysql")

if(PURPLE_FOUND)
	message(STATUS "PURPLE found OK")
	include_directories(${PURPLE_INCLUDE_DIRS})
	message(STATUS ${PURPLE_INCLUDE_DIRS})
else(PURPLE_FOUND)
	message(FATAL_ERROR "No PURPLE")
endif(PURPLE_FOUND)

if(GLIB2_FOUND)
	message(STATUS "GLIB2 found OK")
	include_directories(${GLIB2_INCLUDE_DIRS})
else(GLIB2_FOUND)
	message(FATAL_ERROR "No GLIB2")
endif(GLIB2_FOUND)


if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

#CONFIGURE_FILE(config.h.in config.h)

set(hiicq_SRCS
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
	protocols/icq.cpp
	protocols/facebook.cpp
)

set(hiicq_MOC_HDRS
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
	protocols/abstractprotocol.h
	protocols/icq.h
	protocols/facebook.h
)

add_executable(hiicq ${hiicq_SRCS} ${lrelease_outputs})

FIND_LIBRARY(MYSQL_LIBRARY NAMES mysqlpp)

target_link_libraries(hiicq ${GLOOX_LIBRARIES} ${MYSQL_LIBRARY} ${PURPLE_LIBRARIES} ${GLIB2_LIBRARIES} -export-dynamic)

# install(FILES
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/LICENSE.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/TRANSLATIONS.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/3RDPARTIES.txt
# 	${CMAKE_CURRENT_SOURCE_DIR}/data/defaults.conf
# 	DESTINATION share/fatrat/data
# 	)


