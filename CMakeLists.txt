cmake_minimum_required (VERSION 3.1.0)
project (TagSupporter)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

option(USE_EMBEDDED_SQL_DB  "Use embedded SQL server instead of external one" ON)

find_package(Qt5 COMPONENTS Core Widgets Network Multimedia WebEngineWidgets REQUIRED)
find_package(Taglib REQUIRED)
if(NOT TAGLIB_FOUND)
	message(FATAL_ERROR "Could not find Taglib")
endif(NOT TAGLIB_FOUND)
 
if (USE_EMBEDDED_SQL_DB)
	find_package(MySQL REQUIRED)
else(USE_EMBEDDED_SQL_DB)
	find_package(MariaDB REQUIRED)
	if(NOT MARIADB_FOUND)
		message(FATAL_ERROR "Could not find MariaDB")
	endif(NOT MARIADB_FOUND)
endif(USE_EMBEDDED_SQL_DB)

find_package(ZLIB)
find_package(Threads)

set(CMAKE_CXX_STANDARD 17)
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(TagSupporterSources
	main.cpp
	TagSupporter.cpp
	TagSupporter.h
	TagSupporter.ui
)

add_executable(TagSupporter ${TagSupporterSources})
target_include_directories(TagSupporter PUBLIC "${TAGLIB_INCLUDES}/..")
add_subdirectory(OnlineParsers)
add_subdirectory(Widgets)
add_subdirectory(Tools)

target_link_libraries(TagSupporter ${TAGLIB_LIBRARIES})
target_link_libraries(TagSupporter Qt5::Widgets)
target_link_libraries(TagSupporter Qt5::Core)
target_link_libraries(TagSupporter Qt5::Network)
target_link_libraries(TagSupporter Qt5::Multimedia)
target_link_libraries(TagSupporter Qt5::WebEngineWidgets)

if (MYSQL_FOUND)
	target_include_directories(TagSupporter PUBLIC "${MYSQL_INCLUDES}" )
	target_link_libraries(TagSupporter ${MYSQL_LIB})
	target_compile_definitions(TagSupporter PUBLIC EMBEDDED_SQL_DB)
	if(UNIX AND NOT APPLE)
    	target_link_libraries(TagSupporter rt dl crypt lz4 numa atomic aio)
	endif()
else (MYSQL_FOUND)
	target_include_directories(TagSupporter PUBLIC "${MARIADB_INCLUDES}")
	target_link_libraries(TagSupporter ${MARIADB_LIBRARIES})
endif (MYSQL_FOUND)

target_link_libraries(TagSupporter Threads::Threads)
target_link_libraries(TagSupporter ZLIB::ZLIB)
