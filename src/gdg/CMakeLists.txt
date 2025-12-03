add_library(GDG OBJECT
    framebuffer.c
    framebuffer.h
    gdg.c
    gdg.h
    hwscroll.c
    hwscroll.h
    video.h
    vramctrl.c
    vramctrl.h
)

target_link_libraries(GDG PUBLIC mz800emu_options)
