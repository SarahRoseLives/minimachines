find_path(ENET_INCLUDE_DIR enet/enet.h
    HINTS ${ENET_ROOT_DIR}/include /mingw64/include
)

find_library(ENET_DLL_LIBRARY
    NAMES enet.dll.a
    HINTS ${ENET_ROOT_DIR}/lib /mingw64/lib
)

find_library(ENET_STATIC_LIBRARY
    NAMES libenet.a enet
    HINTS ${ENET_ROOT_DIR}/lib /mingw64/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(enet DEFAULT_MSG ENET_INCLUDE_DIR)

if(enet_FOUND)
    if(NOT TARGET enet::enet)
        set(ENET_SYSTEM_LIBS "")
        if(WIN32)
            set(ENET_SYSTEM_LIBS ws2_32 winmm)
        endif()
        if(ENET_DLL_LIBRARY)
            add_library(enet::enet SHARED IMPORTED)
            set_target_properties(enet::enet PROPERTIES
                IMPORTED_IMPLIB "${ENET_DLL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${ENET_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${ENET_SYSTEM_LIBS}"
            )
            find_file(ENET_DLL_FILE libenet.dll
                HINTS ${ENET_ROOT_DIR}/bin /mingw64/bin
            )
            if(ENET_DLL_FILE)
                set_target_properties(enet::enet PROPERTIES
                    IMPORTED_LOCATION "${ENET_DLL_FILE}"
                )
            else()
                set_target_properties(enet::enet PROPERTIES
                    IMPORTED_LOCATION "${ENET_DLL_LIBRARY}"
                )
            endif()
        elseif(ENET_STATIC_LIBRARY)
            add_library(enet::enet STATIC IMPORTED)
            set_target_properties(enet::enet PROPERTIES
                IMPORTED_LOCATION "${ENET_STATIC_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${ENET_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${ENET_SYSTEM_LIBS}"
            )
        endif()
    endif()
endif()

mark_as_advanced(ENET_INCLUDE_DIR ENET_DLL_LIBRARY ENET_STATIC_LIBRARY ENET_DLL_FILE)
