add_library(CTC8253 OBJECT
    ctc8253.c
    ctc8253.h
    )

target_link_libraries(CTC8253 PUBLIC mz800emu_options)
