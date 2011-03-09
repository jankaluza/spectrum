set(POCO_FOUNDATION "POCO_FOUNDATION-NOTFOUND")
set(POCO_DATA "POCO_DATA-NOTFOUND")
set(POCO_MYSQL "POCO_MYSQL-NOTFOUND")
set(POCO_SQLITE "POCO_SQLITE-NOTFOUND")
set(POCO_ODBC "POCO_ODBC-NOTFOUND")

find_path(LIBPOCO_INCLUDE_DIR NAMES Poco/Poco.h)

find_library(POCO_FOUNDATION NAMES PocoFoundation)

find_library(POCO_DATA NAMES PocoData)

find_library(POCO_MYSQL NAMES PocoMySQL PocoDataMySQL)

find_library(POCO_SQLITE NAMES PocoSQLite PocoDataSQLite)

find_library(POCO_ODBC NAMES PocoODBC PocoDataODBC)

if(LIBPOCO_INCLUDE_DIR AND POCO_DATA AND POCO_FOUNDATION)
	set(LIBPOCO_LIBRARIES ${POCO_DATA} ${POCO_FOUNDATION})
	message(STATUS "Found libPoco: ${LIBPOCO_INCLUDE_DIR}, ${LIBPOCO_LIBRARIES}")
	if(POCO_SQLITE)
		set(LIBPOCO_LIBRARIES ${LIBPOCO_LIBRARIES} ${POCO_SQLITE})
		message(STATUS "Found libPocoSQLite/libPocoDataSQLite: ${POCO_SQLITE}")
		message(STATUS "SQLite storage backend will be available")
		file(APPEND src/transport_config.h "#define WITH_SQLITE 1\n")
	endif(POCO_SQLITE)
	if(POCO_MYSQL)
		set(LIBPOCO_LIBRARIES ${LIBPOCO_LIBRARIES} ${POCO_MYSQL})
		message(STATUS "Found libPocoMySQL/libPocoDataMySQL: ${POCO_MYSQL}")
		message(STATUS "MySQL storage backend will be available")
		file(APPEND src/transport_config.h "#define WITH_MYSQL 1\n")
	endif(POCO_MYSQL)
	if(POCO_ODBC)
		set(LIBPOCO_LIBRARIES ${LIBPOCO_LIBRARIES} ${POCO_ODBC})
		message(STATUS "Found libPocoODBC/libPocoDataODBC: ${POCO_ODBC}")
		message(STATUS "ODBC storage backend will be available")
		file(APPEND src/transport_config.h "#define WITH_ODBC 1\n")
	endif(POCO_ODBC)
else(LIBPOCO_INCLUDE_DIR AND POCO_DATA AND POCO_FOUNDATION)
	message(FATAL_ERROR "libPoco not found.")
endif(LIBPOCO_INCLUDE_DIR AND POCO_DATA AND POCO_FOUNDATION)



mark_as_advanced(LIBPOCO_INCLUDE_DIR LIBPOCO_LIBRARIES)
