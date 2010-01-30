#------------------------------------------------------------------------------
# Desc:
# Tabs: 3
#
# Copyright (c) 2007 Novell, Inc. All Rights Reserved.
#
# This program and the accompanying materials are made available 
# under the terms of the Eclipse Public License v1.0 which
# accompanies this distribution, and is available at 
# http://www.eclipse.org/legal/epl-v10.html
#
# To contact Novell about this file by physical or electronic mail, 
# you may find current contact information at www.novell.com.
#
# $Id$
#
# Author: Andrew Hodgkinson <ahodgkinson@novell.com>
#------------------------------------------------------------------------------

# Include the local modules directory

# Locate GLib files

if( NOT GLIB_FOUND)

	find_path( GLIB_INCLUDE_DIR glib.h 
		PATHS /opt/gtk/include
				/opt/gnome/include
				/usr/include
				/usr/local/include
		PATH_SUFFIXES glib-2.0
		NO_DEFAULT_PATH
	)
	MARK_AS_ADVANCED( GLIB_INCLUDE_DIR)
		
	find_path( GLIB_CONFIG_INCLUDE_DIR glibconfig.h 
		PATHS	/opt/gtk/include
				/opt/gtk/lib
				/opt/gnome/include 
				/opt/gnome/lib
				/usr/include
				/usr/lib
				/usr/local/include
		PATH_SUFFIXES /glib-2.0 /glib-2.0/include
		NO_DEFAULT_PATH
	)
	MARK_AS_ADVANCED( GLIB_CONFIG_INCLUDE_DIR)

	if( NOT GLIB_CONFIG_INCLUDE_DIR)
		message( STATUS "Unable to find GLIB_CONFIG_INCLUDE_DIR")
	endif( NOT GLIB_CONFIG_INCLUDE_DIR)
		
	find_library( GLIB_LIBRARY 
		NAMES glib-2.0 
		PATHS /opt/gtk/lib
				/opt/gnome/lib
				/usr/lib 
				/usr/local/lib
		NO_DEFAULT_PATH
	) 
	MARK_AS_ADVANCED( GLIB_LIBRARY)
		
	if( GLIB_INCLUDE_DIR AND GLIB_CONFIG_INCLUDE_DIR AND GLIB_LIBRARY)
		set( GLIB_FOUND TRUE)
		set( GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIR} ${GLIB_CONFIG_INCLUDE_DIR})
		set( GLIB_LIBRARIES ${GLIB_LIBRARY})
	endif( GLIB_INCLUDE_DIR AND GLIB_CONFIG_INCLUDE_DIR AND GLIB_LIBRARY)
	
	if( GLIB_FOUND)
		if( NOT GLIB_FIND_QUIETLY)
			message( STATUS "Found GLib library: ${GLIB_LIBRARY}")
			message( STATUS "Found GLib inc dirs: ${GLIB_INCLUDE_DIRS}")
		endif( NOT GLIB_FIND_QUIETLY)
	else( GLIB_FOUND)
		if( GLIB_FIND_REQUIRED)
			message( FATAL_ERROR "Could not find GLib")
		else( GLIB_FIND_REQUIRED)
			if( NOT GLIB_FIND_QUIETLY)
				message( STATUS "Could not find GLib")
			endif( NOT GLIB_FIND_QUIETLY)
		endif( GLIB_FIND_REQUIRED)
	endif( GLIB_FOUND)

endif( NOT GLIB_FOUND)
