add_library(ROM_JSS106A OBJECT
    ROM_JSS106A_CGROM.c
    ROM_JSS106A_MZ700.c
    ROM_JSS106A_MZ800.c
    )

target_link_libraries(ROM_JSS106A mz800emu_options)
