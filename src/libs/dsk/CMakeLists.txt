add_library(DSK OBJECT
    dsk.c
    dsk.h
    dsk_tools.c
    dsk_tools.h
    )

target_include_directories(DSK
    PUBLIC 
    .
    )
target_link_libraries(DSK mz800emu_options)
