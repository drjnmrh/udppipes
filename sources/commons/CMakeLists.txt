
set(commons_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/debug.h
    ${CMAKE_CURRENT_SOURCE_DIR}/debug.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/macros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/types.h

    ${CMAKE_CURRENT_SOURCE_DIR}/logger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/queue.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/spinlock.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spinlock.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/threader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/threader.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
)

if (APPLE)
    set(commons_SOURCES ${commons_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/apple/NSLoggerBackend.m)
endif()


add_library(commons STATIC ${commons_SOURCES})


set_property(TARGET commons PROPERTY MODULE_TESTS
    ${CMAKE_CURRENT_LIST_DIR}/tests/test-threader.cpp
)


target_link_libraries(commons PUBLIC "-framework Foundation")


if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    source_group("[sources]" FILES ${commons_SOURCES})
else()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "[sources]" FILES ${commons_SOURCES})
endif()


target_compile_definitions(commons PRIVATE
    $<$<CONFIG:Debug>:DEBUG _DEBUG>
    $<$<CONFIG:Release>:NDEBUG _NDEBUG>
)


if (APPLE)

    set_target_properties(commons PROPERTIES
        XCODE_ATTRIBUTE_CLANG_C_LANGUAGE_STANDARD "c11"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17"

        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug]          "dwarf-with-dsym"
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=MinSizeRel]     "dwarf-with-dsym"
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=RelWithDebInfo] "dwarf-with-dsym"
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release]        "dwarf-with-dsym"

        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Debug]          "YES"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=MinSizeRel]     "NO"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=RelWithDebInfo] "YES"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release]        "NO"
    )

elseif (MSVC)

    set (CMAKE_CXX_STANDARD 17)
    target_compile_options(commons PUBLIC /std:c++17)

else ()

    target_compile_options(commons PUBLIC -std=c++17)

endif()
