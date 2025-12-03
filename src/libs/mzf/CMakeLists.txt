add_library(MZF OBJECT
    mzf.c
    mzf.h
    mzf_tools.c
    mzf_tools.h
    )

target_link_libraries(MZF PUBLIC mz800emu_options)
