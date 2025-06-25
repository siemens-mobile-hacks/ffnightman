cmake_minimum_required(VERSION 3.15)
project(ffnightman VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ffshit REQUIRED)
find_package(fmt REQUIRED)
find_package(Iconv REQUIRED)
find_package(spdlog REQUIRED)
find_package(Git QUIET)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${Iconv_INCLUDE_DIRS}
)

set(SRC_LIST 
    src/help.cpp
    src/extractor.cpp
    src/main.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /Z7 /D_DEBUG /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(VERSION_STRING "${CMAKE_PROJECT_VERSION}")
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_COMMIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(VERSION_STRING "${CMAKE_PROJECT_VERSION}-${GIT_COMMIT_HASH}")
endif()

set(CMAKE_MACOSX_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list( APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib )
list( APPEND CMAKE_INSTALL_RPATH ~/lib )

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_compile_definitions(${PROJECT_NAME} PRIVATE DEF_VERSION_STRING="${VERSION_STRING}")
target_link_libraries(${PROJECT_NAME} PRIVATE ffshit::ffshit fmt::fmt spdlog::spdlog ${Iconv_LIBRARIES})

install(TARGETS ${PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb")
include(deb)
