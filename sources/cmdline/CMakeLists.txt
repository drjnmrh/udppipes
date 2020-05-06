
set(udpcmd_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/testapi/testapi.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/main-cmdline.cpp
)


get_property(commons_TESTS TARGET commons PROPERTY MODULE_TESTS)
get_property(sockets_TESTS TARGET sockets PROPERTY MODULE_TESTS)


if (IOS)
    add_executable(udpcmd MACOSX_BUNDLE ${udpcmd_SOURCES} ${commons_TESTS} ${sockets_TESTS})
else()
    add_executable(udpcmd ${udpcmd_SOURCES} ${commons_TESTS} ${sockets_TESTS})
endif()


if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    source_group("[sources]" FILES ${udpcmd_SOURCES})

    source_group("[tests]\\commons" FILES ${commons_TESTS})
    source_group("[tests]\\sockets" FILES ${sockets_TESTS})
else()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "[sources]" FILES ${udpcmd_SOURCES})

    source_group("[tests]\\commons" FILES ${commons_TESTS})
    source_group("[tests]\\sockets" FILES ${sockets_TESTS})
endif()


target_link_libraries(udpcmd sockets)


target_compile_definitions(udpcmd PRIVATE
    $<$<CONFIG:Debug>:DEBUG _DEBUG>
    $<$<CONFIG:Release>:NDEBUG _NDEBUG>
)


target_include_directories(udpcmd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/testapi
)


if (APPLE)

    set_target_properties(udpcmd PROPERTIES
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

        XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    )

    if (IOS)
        message(STATUS "Bundle   : ${IPHONE_BUNDLEID}")
        message(STATUS "Dev Team : ${IPHONE_DEVTEAM}")
        message(STATUS "Sign Id  : ${IPHONE_SIGNID}")

        set_target_properties(udpcmd PROPERTIES
            MACOSX_BUNDLE TRUE

            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${IPHONE_BUNDLEID}

            MACOSX_BUNDLE_BUNDLE_NAME udpcmd
            MACOSX_BUNDLE_EXECUTABLE_NAME udpcmd
            MACOSX_BUNDLE_INFO_STRING "testing app for ${module}"
		    MACOSX_BUNDLE_GUI_IDENTIFIER ${IPHONE_BUNDLEID}
		    MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
            MACOSX_BUNDLE_COPYRIGHT "Copyright @ stoned_fox"

            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${IPHONE_SIGNID}
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${IPHONE_DEVTEAM}
        )

    else ()
        set_target_properties(udpcmd PROPERTIES
            XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 10.12 # @TODO(stoned_fox): make possible to change this using arguments of the script
        )
    endif()

elseif (MSVC)

    set (CMAKE_CXX_STANDARD 17)
    target_compile_options(udpcmd PUBLIC /std:c++17)

else ()

    target_compile_options(udpcmd PUBLIC -std=c++17)

endif()


add_test(cmdline_utests udpcmd)
