target_include_directories(mz800emu_options INTERFACE 
    .
    z80ex/include
    z80ex
    cmt
    ui
    cfgfile
    joy
    )

add_subdirectory(cmt)
add_subdirectory(z80ex)
add_subdirectory(ctc8253)
add_subdirectory(fdc)
add_subdirectory(gdg)
add_subdirectory(ide8)
add_subdirectory(joy)
add_subdirectory(memory)
add_subdirectory(debugger)
add_subdirectory(pioz80)
add_subdirectory(pio8255)
add_subdirectory(psg)
add_subdirectory(qdisk)
add_subdirectory(ramdisk)
add_subdirectory(unicard)
add_subdirectory(version_check)
add_subdirectory(ui)
add_subdirectory(cfgfile)
add_subdirectory(iface_sdl)
add_subdirectory(libs)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/build_time.c WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND sh tools/create_buildtime.sh)

add_executable(mz800emu WIN32
    windows_icon/app.rc
    build_time.c
    audio.c
    audio.h
    build_time.h
    cfgmain.c
    cfgmain.h
    debug.h
    display.c
    display.h
    fs_layer.c
    fs_layer.h
    main.c
    main.h
    mz800.c
    mz800emu_cfg.h
    mz800.h
    port.c
    port.h
    sharpmz_ascii.c
    sharpmz_ascii.h

    $<TARGET_OBJECTS:CPU>
    $<TARGET_OBJECTS:CMT>
    $<TARGET_OBJECTS:ConfigFile>
    $<TARGET_OBJECTS:InterfaceSDL>
    $<TARGET_OBJECTS:CMTstream>
    $<TARGET_OBJECTS:CMTtool>
    $<TARGET_OBJECTS:DSK>
    $<TARGET_OBJECTS:ENDIANITY>
    $<TARGET_OBJECTS:GENERIC_DRIVER>
    $<TARGET_OBJECTS:MZtape>
    $<TARGET_OBJECTS:MZF>
    $<TARGET_OBJECTS:QD>
    $<TARGET_OBJECTS:WAV>
    $<TARGET_OBJECTS:ZXtape>

    $<TARGET_OBJECTS:CTC8253>
    $<TARGET_OBJECTS:FDC>
    $<TARGET_OBJECTS:GDG>
    $<TARGET_OBJECTS:IDE8>
    $<TARGET_OBJECTS:JOY>
    $<TARGET_OBJECTS:Memory>
    $<TARGET_OBJECTS:ROM>
    $<TARGET_OBJECTS:ROM_WILLY>
    $<TARGET_OBJECTS:ROM_JSS103>
    $<TARGET_OBJECTS:ROM_JSS105C>
    $<TARGET_OBJECTS:ROM_JSS106A>
    $<TARGET_OBJECTS:ROM_JSS108C>

    $<TARGET_OBJECTS:MZ800Debugger>
    $<TARGET_OBJECTS:PIOZ80>
    $<TARGET_OBJECTS:PIO8255>
    $<TARGET_OBJECTS:PSG>
    $<TARGET_OBJECTS:QDISK>
    $<TARGET_OBJECTS:Ramdisk>
    $<TARGET_OBJECTS:Unicard>
    $<TARGET_OBJECTS:VersionCheck>

    $<TARGET_OBJECTS:UI>
    $<TARGET_OBJECTS:debugger_ui>
    $<TARGET_OBJECTS:dsk_tool_ui>
    $<TARGET_OBJECTS:generic_driver_ui>
    $<TARGET_OBJECTS:tool_ui>
    $<TARGET_OBJECTS:vkbd_ui>
    )

target_link_libraries(mz800emu PRIVATE mz800emu_options)
# this is to make glade callbacks work 
set_target_properties(mz800emu PROPERTIES 
    ENABLE_EXPORTS TRUE
    ) 
