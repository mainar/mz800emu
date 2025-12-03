add_library(FDC OBJECT
    fdc.c
    fdc.h
    wd279x.c
    wd279x.h
    )

target_link_libraries(FDC PUBLIC mz800emu_options)
