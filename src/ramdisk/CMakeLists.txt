add_library(Ramdisk OBJECT
    ramdisk.c
    ramdisk.h
    )

target_link_libraries(Ramdisk PUBLIC mz800emu_options)
