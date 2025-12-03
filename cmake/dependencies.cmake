# somewhat linux centric atm 

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0 IMPORTED_TARGET)

pkg_check_modules(LIBSOUP REQUIRED libsoup-2.4 IMPORTED_TARGET)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0 IMPORTED_TARGET)

pkg_check_modules(SDL2 sdl2 IMPORTED_TARGET GLOBAL)

#if not found let's try something else.
if(NOT SDL2_FOUND)
    if(CMAKE_CROSSCOMPILING)
        message("looking for sdl2-config in ${CMAKE_FIND_ROOT_PATH}")
        find_program(SDL2CONFIG_EXECUTABLE NAMES ${COMPILER_PREFIX}-sdl2-config
            PATHS ${CMAKE_FIND_ROOT_PATH}/bin
            )
    else()
        find_program(SDL2CONFIG_EXECUTABLE NAMES sdl2-config)
    endif()
    if(SDL2CONFIG_EXECUTABLE)
        message("found ${SDL2CONFIG_EXECUTABLE}")
    else()
        message(FATAL_ERROR "couldn't find sdl2-config ${SDL2CONFIG_EXECUTABLE}" )
    endif()

    execute_process(COMMAND ${SDL2CONFIG_EXECUTABLE} --libs OUTPUT_VARIABLE _SDL2_LIBS)
    execute_process(COMMAND ${SDL2CONFIG_EXECUTABLE} --cflags OUTPUT_VARIABLE _SDL2_FLAGS)

    string(REPLACE "\ " ";" __SDL2_LIBS ${_SDL2_LIBS})
    string(REPLACE "\ " ";" SDL2_FLAGS ${_SDL2_FLAGS})

    foreach(LIB ${__SDL2_LIBS})
        unset(_LIB)
        string(REGEX MATCH "-l.*" _LIB "${LIB}")
        if(_LIB)
            string(REPLACE "-l" "" _LIB "${LIB}")
            string(STRIP ${_LIB} _LIB)
            list(APPEND SDL2_LIBS ${_LIB})
        endif()

        #possible locations
        unset(_LIBLOC)
        string(REGEX MATCH "-L.*" _LIBLOC "${LIB}")
        if(_LIBLOC) 
            string(REPLACE "-L" "" _LIBLOC "${LIB}")
            string(STRIP ${_LIBLOC} _LIBLOC)
            list(APPEND SDL2_LIBLOCS ${_LIBLOC})
        endif()
    endforeach()

    add_library(SDL2::SDL2 INTERFACE IMPORTED GLOBAL)
    set_property(TARGET SDL2::SDL2 PROPERTY
        INTERFACE_COMPILE_OPTIONS "${SDL2_FLAGS}"
        )
    set_property(TARGET SDL2::SDL2 PROPERTY
        INTERFACE_LINK_LIBRARIES "${SDL2_LIBS}"
        )
    if(SDL2_LIBLOCS) 
        set_property(TARGET SDL2::SDL2 PROPERTY
            INTERFACE_LINK_DIRECTORIES "${SDL2_LIBLOCS}"
            )
    endif()
else()
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()
