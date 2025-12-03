add_library(CPU
    include/z80ex.h
    include/z80ex_common.h
    include/z80ex_dasm.h

    z80ex.c
    z80ex_dasm.c
    )

target_include_directories(CPU
    PUBLIC 
    include
    )

