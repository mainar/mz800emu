add_library(WAV OBJECT
    wav.c
    wav.h
    )

target_link_libraries(WAV PUBLIC mz800emu_options)
