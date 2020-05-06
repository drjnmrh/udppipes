cmake_minimum_required(VERSION 3.8)


# @MARK - Setup options

option(WITHUI    "generate project with UI" OFF)

if (IOS)
    option(IPHONE_BUNDLEID "iPhone bundle id" OFF)
    option(IPHONE_DEVTEAM "iPhone dev team" OFF)
    option(IPHONE_SIGNID "iPhone Sign Id" OFF)
endif()

# @MARK - Setup modules


enable_testing()


project(udp LANGUAGES C CXX Swift)


if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(${archvar} ${ANDROID_ABI})
    if (NOT DEFINED ANDROID)
        set(ANDROID TRUE)
    elseif (NOT ANDROID)
        set(ANDROID TRUE)
    endif()
elseif (IOS)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if (NOT DEFINED MACOSX)
        set(MACOSX TRUE)
    elseif (NOT MACOSX)
        set(MACOSX TRUE)
    endif()
elseif (UNIX)
    set(LINUX TRUE)
endif()


define_property(TARGET PROPERTY MODULE_TESTS
    BRIEF_DOCS "list of the module test sources"
    FULL_DOCS  "list of the module test sources"
)


include_directories(${CMAKE_SOURCE_DIR}/sources)


add_subdirectory(${CMAKE_SOURCE_DIR}/sources/commons)
add_subdirectory(${CMAKE_SOURCE_DIR}/sources/sockets)



if (NOT ${WITHUI})
    add_subdirectory(${CMAKE_SOURCE_DIR}/sources/cmdline)
elseif (MACOSX)
    add_subdirectory(${CMAKE_SOURCE_DIR}/sources/appkit)
else ()
    message(FATAL_ERROR "platform UI not supported!")
endif()

