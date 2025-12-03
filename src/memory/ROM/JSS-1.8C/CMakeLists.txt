add_library(ROM_JSS108C OBJECT
    ROM_JSS108C_CGROM.c
    ROM_JSS108C_MZ700.c
    ROM_JSS108C_MZ800.c
    )

target_link_libraries(ROM_JSS108C mz800emu_options)
