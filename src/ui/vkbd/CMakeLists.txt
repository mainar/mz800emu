add_library(vkbd_ui OBJECT 
    ui_vkbd_autotype.c
    ui_vkbd_autotype.h
    ui_vkbd.c
    ui_vkbd.h
    ui_vkbd_linux_x11.c
    ui_vkbd_linux_x11.h
    ui_vkbd_scancode.h
    ui_vkbd_windows.c
    ui_vkbd_windows.h
    windows_virtual_key_codes.h
    )
target_link_libraries(vkbd_ui mz800emu_options)
