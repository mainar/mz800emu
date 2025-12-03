add_library(ROM OBJECT
    ROM_CGROM.c
    ROM_MZ700.c
    ROM_MZ800.c
    )

target_link_libraries(ROM mz800emu_options)

add_subdirectory(JSS-1.3)
add_subdirectory(JSS-1.5C)
add_subdirectory(JSS-1.6A)
add_subdirectory(JSS-1.8C)
add_subdirectory(WILLY)
