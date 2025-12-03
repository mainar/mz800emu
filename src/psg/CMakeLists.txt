add_library(PSG OBJECT
    psg.c
    psg.h
    )

target_link_libraries(PSG PUBLIC mz800emu_options)
