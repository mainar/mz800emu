add_library(ConfigFile OBJECT
    cfgcommon.h
    cfgelement.c
    cfgelement.h
    cfgmodule.c
    cfgmodule.h
    cfgroot.c
    cfgroot.h
    cfgtools.c
    cfgtools.h
    )

target_include_directories(ConfigFile 
    PUBLIC .
    .. 
    )

target_link_libraries(ConfigFile mz800emu_options)
