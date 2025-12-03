add_library(CMTstream OBJECT
    cmt_bitstream.c
    cmt_stream.c
    cmt_vstream.c
    )

target_include_directories(CMTstream
    PUBLIC 
    .
    )
target_link_libraries(CMTstream mz800emu_options)
