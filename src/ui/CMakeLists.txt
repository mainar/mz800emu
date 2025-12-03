add_library(UI OBJECT
    ui_cmt.c
    ui_cmt.h
    ui_cmt_tape.c
    ui_cmt_tape.h
    ui_display.c
    ui_display.h
    ui_fcbutton.c
    ui_fcbutton.h
    ui_fdc.c
    ui_fdc.h
    ui_file_chooser.c
    ui_file_chooser.h
    ui_hexeditable.c
    ui_hexeditable.h
    ui_ide8.c
    ui_ide8.h
    ui_joy.c
    ui_joy.h
    ui_main.c
    ui_main.h
    ui_memext.c
    ui_memext.h
    ui_qdisk.c
    ui_qdisk.h
    ui_ramdisk.c
    ui_ramdisk.h
    ui_rom.c
    ui_rom.h
    ui_unicard.c
    ui_unicard.h
    ui_utils.c
    ui_utils.h
    ui_version_check.c
    ui_version_check.h
    ui_audio.c
    ui_audio.h
)

target_include_directories(UI 
    PUBLIC 
    .
    )
target_link_libraries(UI mz800emu_options)

add_subdirectory(debugger)
add_subdirectory(dsk_tool)
add_subdirectory(generic_driver)
add_subdirectory(tools)
add_subdirectory(vkbd)

