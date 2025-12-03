add_library(ZXtape OBJECT
    zxtape.c
    zxtape.h
    )
target_link_libraries(ZXtape PUBLIC mz800emu_options)
