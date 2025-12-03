add_library(QDISK OBJECT
    qdisk.c
    qdisk.h
    )

target_link_libraries(QDISK PUBLIC mz800emu_options)
