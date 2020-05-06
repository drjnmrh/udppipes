
set(sockets_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/udpaddress.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udpaddress.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/udpdgram.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udpdgram.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/udpengine.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udpengine.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/udppipe.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udppipe.cpp
)


add_library(sockets STATIC ${sockets_SOURCES})


set_property(TARGET sockets PROPERTY MODULE_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-udpengine.cpp
)


if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    source_group("[sources]" FILES ${sockets_SOURCES})
else()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "[sources]" FILES ${sockets_SOURCES})
endif()


target_link_libraries(sockets PUBLIC commons)


target_compile_definitions(sockets PRIVATE
    $<$<CONFIG:Debug>:DEBUG _DEBUG>
    $<$<CONFIG:Release>:NDEBUG _NDEBUG>
)


if (APPLE)

    set_target_properties(sockets PROPERTIES
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
    target_compile_options(sockets PUBLIC /std:c++17)

else ()

    target_compile_options(sockets PUBLIC -std=c++17)

endif()
