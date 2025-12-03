add_library(PIO8255 OBJECT
    pio8255.c
    pio8255.h
    )

target_link_libraries(PIO8255 PUBLIC mz800emu_options)
