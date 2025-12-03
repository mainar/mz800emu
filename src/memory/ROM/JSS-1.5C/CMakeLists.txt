add_library(ROM_JSS105C OBJECT
    ROM_JSS105C_CGROM.c
    ROM_JSS105C_MZ700.c
    ROM_JSS105C_MZ800.c
    )

target_link_libraries(ROM_JSS105C mz800emu_options)
