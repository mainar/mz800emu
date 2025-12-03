add_library(debugger_ui OBJECT
    ui_breakpoints.c
    ui_breakpoints.h
    ui_dbg_memext.c
    ui_dbg_memext.h
    ui_debugger.c
    ui_debugger_callbacks.c
    ui_debugger_callbacks.h
    ui_debugger.h
    ui_debugger_iasm.c
    ui_debugger_iasm.h
    ui_dissassembler.c
    ui_dissassembler.h
    ui_membrowser.c
    ui_membrowser.h
    ui_memload.c
    ui_memload.h
    ui_memsave.c
    ui_memsave.h
    )

target_link_libraries(debugger_ui mz800emu_options)
