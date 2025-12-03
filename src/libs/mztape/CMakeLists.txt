add_library(MZtape OBJECT
    cmtspeed.c
    cmtspeed.h
    mztape.c
    mztape.h
    )

target_link_libraries(MZtape PUBLIC mz800emu_options)
