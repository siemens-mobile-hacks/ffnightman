cmake_minimum_required(VERSION 3.5)
project(ffnightman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ffshit REQUIRED)
find_package(fmt REQUIRED)
find_package(Iconv REQUIRED)
find_package(spdlog REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${Iconv_INCLUDE_DIRS}
)

set(SRC_LIST 
    src/help.cpp
    src/extractor.cpp
    src/main.cpp)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ffshit::ffshit fmt::fmt spdlog::spdlog ${Iconv_LIBRARIES})
