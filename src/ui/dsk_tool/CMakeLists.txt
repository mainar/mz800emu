add_library(dsk_tool_ui OBJECT
    ui_dsk_tool.c
    ui_dsk_tool.h
    )

target_link_libraries(dsk_tool_ui mz800emu_options)
