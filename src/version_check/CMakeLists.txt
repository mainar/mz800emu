add_library(VersionCheck OBJECT
    version_check.c
    version_check.h
    version_xml_parser.c
    version_xml_parser.h
)

target_link_libraries(VersionCheck mz800emu_options)
