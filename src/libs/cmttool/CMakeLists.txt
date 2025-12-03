add_library(CMTtool OBJECT
    cmttool.h
    cmttool_fastipl.h
    cmttool_turbo.h
    cmttool.c
    cmttool_fastipl.c
    cmttool_turbo.c
    )

target_include_directories(CMTtool
    PUBLIC 
    .
    )
target_link_libraries(CMTtool mz800emu_options)
