add_library(InterfaceSDL
    iface_sdl.h
    iface_sdl_audio.h
    iface_sdl_joy.h
    iface_sdl_keyboard.h
    iface_sdl_log.h

    iface_sdl.c
    iface_sdl_audio.c
    iface_sdl_joy.c
    iface_sdl_keyboard.c
    iface_sdl_log.c
    )

target_include_directories(InterfaceSDL
    PUBLIC 
    .
    ..
    )

target_link_libraries(InterfaceSDL PUBLIC mz800emu_options)
