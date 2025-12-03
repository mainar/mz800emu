add_library(QD OBJECT
    qd.c
    qd.h
    )

target_link_libraries(QD PUBLIC mz800emu_options)
