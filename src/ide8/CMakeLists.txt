add_library(IDE8 OBJECT
    ide8.c
    ide8.h
    )

target_link_libraries(IDE8 PUBLIC mz800emu_options)
