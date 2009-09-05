project(migratedb)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


set(CMAKE_MODULE_PATH "cmake_modules")
include_directories(src)

set(libpoco_DIR ${CMAKE_MODULE_PATH})
find_package(libpoco REQUIRED)
set(glib_DIR ${CMAKE_MODULE_PATH})
find_package(glib REQUIRED)

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

set(migratedb_SRCS
	main.cpp
)

set(migratedb_MOC_HDRS
	main.h
)

add_executable(migratedb ${migratedb_SRCS} ${lrelease_outputs})

target_link_libraries(migratedb ${LIBPOCO_LIBRARIES} ${GLIB_LIBRARIES} -export-dynamic)

