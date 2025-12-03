add_library(generic_driver_ui OBJECT
    ui_file_driver.c
    ui_file_driver.h
    ui_memory_driver.c
    ui_memory_driver.h
    )

target_link_libraries(generic_driver_ui mz800emu_options)
