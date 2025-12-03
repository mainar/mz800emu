add_library(ROM_WILLY OBJECT
    ROM_WILLY_en_CGROM.c
    ROM_WILLY_en_MZ800.c
    ROM_WILLY_ge_CGROM.c
    ROM_WILLY_ge_MZ800.c
    ROM_WILLY_jap_CGROM.c
    ROM_WILLY_jap_MZ800.c
    ROM_WILLY_MZ700.c
)

target_link_libraries(ROM_WILLY mz800emu_options)
