add_library(MZ800Debugger OBJECT
    breakpoints.c
    breakpoints.h
    debugger.c
    debugger.h
    inline_asm.c
    inline_asm.h
    inline_asm_opcodes.h
    )

target_link_libraries(MZ800Debugger PUBLIC mz800emu_options)
