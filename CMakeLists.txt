cmake_minimum_required (VERSION 3.1.0)
project (TagSupporter)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Widgets Network Multimedia WebEngineWidgets REQUIRED)
find_package(Taglib REQUIRED)
find_package(unofficial-libmariadb REQUIRED)
find_package(ZLIB)
find_package(Threads)

if(Qt5_FOUND AND WIN32 AND TARGET Qt5::qmake)
    get_target_property(qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)
    message(NOTICE "Using Qt5::qmake from ${qt5_qmake_location} to determine host tool path")

    execute_process(
        COMMAND "${qt5_qmake_location}" -query QT_HOST_BINS
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt5_tools_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(qt5_debug_tools_prefix "${qt5_tools_prefix}/../debug/bin")
    
    find_program(TOOL_WINDEPLOYQT NAMES windeployqt PATHS "${qt5_tools_prefix}")
    find_program(TOOL_WINDEPLOYQT_DEBUG NAMES windeployqt PATHS "${qt5_debug_tools_prefix}")
    find_program(TOOL_QMAKE NAMES qmake PATHS "${qt5_tools_prefix}")
    find_program(TOOL_QMAKE_DEBUG NAMES qmake PATHS "${qt5_debug_tools_prefix}")
    
    if(NOT TOOL_QMAKE OR NOT TOOL_WINDEPLOYQT OR NOT TOOL_QMAKE_DEBUG OR NOT TOOL_WINDEPLOYQT_DEBUG)
        message(FATAL_ERROR "Could not find qmake or windeployqt in tool path ${qt5_tools_prefix}")
    endif()
    execute_process(
        COMMAND "${TOOL_QMAKE}" -query QT_INSTALL_BINS
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt5_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND "${TOOL_QMAKE_DEBUG}" -query QT_INSTALL_BINS
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt5_debug_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(NOTICE "Looking for Qt binaries in ${qt5_install_prefix}")
endif()

set(CMAKE_CXX_STANDARD 17)

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
target_link_libraries(TagSupporter libmariadb)

target_link_libraries(TagSupporter Threads::Threads)
target_link_libraries(TagSupporter ZLIB::ZLIB)

if(TOOL_WINDEPLOYQT)
    message(NOTICE "Deploying with windeployqt")
    add_custom_command(TARGET TagSupporter
        POST_BUILD
        COMMAND set "PATH=$<$<CONFIG:Debug>:${qt5_debug_install_prefix}>$<$<NOT:$<CONFIG:Debug>>:${qt5_install_prefix}>;%PATH%"
        COMMAND "$<$<CONFIG:Debug>:${TOOL_WINDEPLOYQT_DEBUG}>$<$<NOT:$<CONFIG:Debug>>:${TOOL_WINDEPLOYQT}>" "--no-angle" "$<TARGET_FILE_DIR:TagSupporter>/$<TARGET_FILE_NAME:TagSupporter>"
    )
endif()
