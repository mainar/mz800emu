add_library(JOY OBJECT
    joy.c
    joy.h
    )

target_link_libraries(JOY PUBLIC mz800emu_options)
