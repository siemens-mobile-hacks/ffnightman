cmake_minimum_required(VERSION 3.15)
project(ffnightman VERSION 0.0.1 LANGUAGES CXX)

option(BUILD_DEV "Development build" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MINGW)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    endif()
endif()

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
    if(MINGW)
        set(CMAKE_CXX_FLAGS_DEBUG "-g -static -static-libgcc -static-libstdc++")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -static -static-libgcc -static-libstdc++")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /Z7 /D_DEBUG /Od")
        set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    endif()
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(VERSION_PATCH "${PROJECT_VERSION_PATCH}")

if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_COMMIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(APPEND VERSION_PATCH "-${GIT_COMMIT_HASH}")
endif()

if(BUILD_DEV)
    string(APPEND VERSION_PATCH "-unstable")
endif()

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    endif()
    list( APPEND CMAKE_INSTALL_RPATH ~/lib )
endif()

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_compile_definitions(${PROJECT_NAME} PRIVATE DEF_VERSION_STRING="${VERSION_STRING}")
target_link_libraries(${PROJECT_NAME} PRIVATE ffshit::ffshit fmt::fmt spdlog::spdlog ${Iconv_LIBRARIES})

install(TARGETS ${PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb")
include(deb)
