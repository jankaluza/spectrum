project(spectrum)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH "../cmake_modules")

set(purple_DIR "${CMAKE_SOURCE_DIR}/../cmake_modules")
find_package(purple REQUIRED)
set(glib_DIR "${CMAKE_SOURCE_DIR}/../cmake_modules")
find_package(glib REQUIRED)

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

include_directories(${PURPLE_INCLUDE_DIR})

if(GLIB2_FOUND)
	include_directories(${GLIB2_INCLUDE_DIR})
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
	ADD_DEFINITIONS(-Werror=format-security)
	ADD_DEFINITIONS(-Winit-self)
	ADD_DEFINITIONS(-Wmissing-declarations)
	ADD_DEFINITIONS(-Wmissing-noreturn)
	ADD_DEFINITIONS(-Wpointer-arith)
	ADD_DEFINITIONS(-Wreorder)
	ADD_DEFINITIONS(-Woverloaded-virtual)
	ADD_DEFINITIONS(-Wsign-promo)
	ADD_DEFINITIONS(-Wundef -Wunused)
#	ADD_DEFINITIONS(-Weffc++)
#	ADD_DEFINITIONS(-pedantic)
	message(STATUS "Build type is set to Debug")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

message(STATUS "Protocols will be installed into: " ${CMAKE_INSTALL_PREFIX})

add_library(prpldict SHARED dict/dict.c)
target_link_libraries(prpldict ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} -export-dynamic)
set_target_properties(prpldict PROPERTIES COMPILE_FLAGS -DPURPLE_PLUGINS)

add_library(prplrss SHARED rss/rss.c)
target_link_libraries(prplrss ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${GETTEXTPO_LIBRARY} -lmrss -export-dynamic)
set_target_properties(prplrss PROPERTIES COMPILE_FLAGS -DPURPLE_PLUGINS)

INSTALL(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/libprpldict.so
	${CMAKE_CURRENT_SOURCE_DIR}/libprplrss.so
	DESTINATION lib/purple-2
	)
