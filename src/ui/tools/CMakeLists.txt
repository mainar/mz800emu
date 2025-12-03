add_library(tool_ui OBJECT
    ui_tool_pixbuf.c
    ui_tool_pixbuf.h
    )

target_link_libraries(tool_ui mz800emu_options)
