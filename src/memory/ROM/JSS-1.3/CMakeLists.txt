add_library(ROM_JSS103 OBJECT
    ROM_JSS103_CGROM.c
    ROM_JSS103_MZ700.c
    ROM_JSS103_MZ800.c
    )

target_link_libraries(ROM_JSS103 mz800emu_options)
