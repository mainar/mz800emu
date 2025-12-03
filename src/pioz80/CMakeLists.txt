add_library(PIOZ80 OBJECT
    pioz80.c
    pioz80.h
    )

target_link_libraries(PIOZ80 PUBLIC mz800emu_options)
