add_library(GENERIC_DRIVER OBJECT
    generic_driver.c
    generic_driver.h
    )
target_link_libraries(GENERIC_DRIVER PUBLIC 
    mz800emu_options)
