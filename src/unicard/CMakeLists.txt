add_library(Unicard OBJECT
    ff_open_mode.h
    ff_result.h
    MGR_MZF.c
    MGR_MZF.h
    MZFLOADER_MZQ.c
    MZFLOADER_MZQ.h
    unicard.c
    unicard.h
    unimgr.c
    unimgr_commands.h
    unimgr.h
    )

target_link_libraries(Unicard PUBLIC mz800emu_options)
