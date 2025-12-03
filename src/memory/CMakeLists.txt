add_library(Memory OBJECT
    memext.c
    memext.h
    memory.c
    memory.h
    rom.c
    rom.h
    )

target_link_libraries(Memory PUBLIC mz800emu_options)

add_subdirectory(ROM)
