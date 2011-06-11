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
set(event_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(event)
set(magick_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(magick)

# correctly find the python interpreter
include(FindPythonInterp)
 
execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Python site-packages directory: "  ${PYTHON_SITE_PACKAGES})

include_directories(${GLOOX_INCLUDE_DIR})
include_directories(${PURPLE_INCLUDE_DIR})
include_directories(${EVENT_INCLUDE_DIRS})
include_directories(${LIBPOCO_INCLUDE_DIR})

if(GLIB2_FOUND)
	include_directories(${GLIB2_INCLUDE_DIR})
	message( STATUS "Found GLIB2: ${GLIB2_INCLUDE_DIR} ${GLIB2_LIBRARIES}")
else(GLIB2_FOUND)
	message(FATAL_ERROR "No GLIB2")
endif(GLIB2_FOUND)

if(Magick_LIBRARY AND Magick_INCLUDE_DIR)
	message( STATUS "Found Magick++: " ${Magick_INCLUDE_DIR} ${Magick_LIBRARY})
	include_directories(${Magick_INCLUDE_DIR})
	file(APPEND src/transport_config.h "#define WITH_IMAGEMAGICK 1\n")
else(Magick_LIBRARY AND Magick_INCLUDE_DIR)
	message( STATUS "Magick++ not found - avatars synchronization don't have to work!!!")
    SET(Magick_LIBRARY "")
endif(Magick_LIBRARY AND Magick_INCLUDE_DIR)


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

FILE(GLOB spectrum_SRCS src/*.cpp src/protocols/*.cpp src/adhoc/*.cpp)

add_executable(spectrum ${spectrum_SRCS} ${lrelease_outputs})

if(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum ${WITH_STATIC_GLOOX} -lgnutls ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${LIBPOCO_LIBRARIES} ${EVENT_LIBRARIES} ${Magick_LIBRARY} -export-dynamic)
else(DEFINED WITH_STATIC_GLOOX)
	target_link_libraries(spectrum ${GLOOX_LIBRARIES} ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${LIBPOCO_LIBRARIES} ${EVENT_LIBRARIES} ${Magick_LIBRARY} -export-dynamic)
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
		file(APPEND src/transport_config.h "#define VERSION \"1.4.9-git-" ${GIT_REVISION} "\"\n")
	else (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
		file(APPEND src/transport_config.h "#define VERSION \"1.4.8\"\n")
	endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
endif (SPECTRUM_VERSION)

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
                                COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_gmoFile} ${_poFile}
                                DEPENDS ${_poFile})

                        ADD_CUSTOM_COMMAND(TARGET xgettext
                                COMMAND msgmerge ${_poFile} ${CMAKE_CURRENT_SOURCE_DIR}/po/messages.po | msgattrib --no-obsolete > ${_poFile}.new && mv ${_poFile}.new ${_poFile}
								)
                        INSTALL(FILES ${_gmoFile} DESTINATION share/locale/${_langCode}/LC_MESSAGES/ RENAME ${catalogname}.mo)
                ENDIF( _langCode )
 
        ENDFOREACH(_poFile ${PO_FILES})
 
ENDIF(NOT GETTEXT_MSGFMT_EXECUTABLE)

ADD_CUSTOM_TARGET (man ALL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/spectrumctl/make_man_page.py > ${CMAKE_CURRENT_SOURCE_DIR}/man/spectrumctl.8)


INSTALL(TARGETS spectrum
    RUNTIME DESTINATION bin
    )



INSTALL(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/spectrum.cfg
	RENAME spectrum.cfg.example
	DESTINATION /etc/spectrum
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


message("Supported features")
message("--------------------------------")

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Debugging symbols:           yes")
else(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Debugging symbols:           no (run cmake with -DCMAKE_BUILD_TYPE=Debug option)")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

IF(GETTEXT_MSGFMT_EXECUTABLE)
	message("Translations:                yes")
ELSE(GETTEXT_MSGFMT_EXECUTABLE)
	message("Translations:                no (install msgfmt from gettext package)")
endif(GETTEXT_MSGFMT_EXECUTABLE)

if(Magick_LIBRARY AND Magick_INCLUDE_DIR)
	message("Avatars resizing:            yes")
else(Magick_LIBRARY AND Magick_INCLUDE_DIR)
	message("Avatars resizing:            no (install Magick++)")
endif(Magick_LIBRARY AND Magick_INCLUDE_DIR)

IF(EVENT_INCLUDE_DIRS AND EVENT_LIBRARIES)
SET(HAVE_EVENT TRUE)
	message("libev (epoll, kpoll, ...):   yes")
ELSE(EVENT_INCLUDE_DIRS AND EVENT_LIBRARIES)
	message("libev (epoll, kpoll, ...):   no (install libev)")
ENDIF(EVENT_INCLUDE_DIRS AND EVENT_LIBRARIES)

if(POCO_SQLITE)
	message("SQLite3:                     yes")
else(POCO_SQLITE)
	message("SQLite3:                     no (install libPocoSQLite/libPocoDataSQLite)")
endif(POCO_SQLITE)
if(POCO_MYSQL)
	message("MySQL:                       yes")
else(POCO_MYSQL)
	message("MySQL:                       no (install libPocoMySQL/libPocoDataMySQL)")
endif(POCO_MYSQL)

message("--------------------------------")
