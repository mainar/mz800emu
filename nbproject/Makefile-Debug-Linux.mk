#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug-Linux
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/audio.o \
	${OBJECTDIR}/src/build_time.o \
	${OBJECTDIR}/src/cfgfile/cfgelement.o \
	${OBJECTDIR}/src/cfgfile/cfgmodule.o \
	${OBJECTDIR}/src/cfgfile/cfgroot.o \
	${OBJECTDIR}/src/cfgfile/cfgtools.o \
	${OBJECTDIR}/src/cfgmain.o \
	${OBJECTDIR}/src/cmt/cmt.o \
	${OBJECTDIR}/src/cmt/cmt_mzf.o \
	${OBJECTDIR}/src/cmt/cmt_mzftape.o \
	${OBJECTDIR}/src/cmt/cmt_save.o \
	${OBJECTDIR}/src/cmt/cmt_tap.o \
	${OBJECTDIR}/src/cmt/cmt_wav.o \
	${OBJECTDIR}/src/cmt/cmtext.o \
	${OBJECTDIR}/src/cmt/cmtext_block.o \
	${OBJECTDIR}/src/cmt/cmtext_container.o \
	${OBJECTDIR}/src/cmt/cmthack.o \
	${OBJECTDIR}/src/ctc8253/ctc8253.o \
	${OBJECTDIR}/src/debugger/breakpoints.o \
	${OBJECTDIR}/src/debugger/debugger.o \
	${OBJECTDIR}/src/debugger/inline_asm.o \
	${OBJECTDIR}/src/display.o \
	${OBJECTDIR}/src/fdc/fdc.o \
	${OBJECTDIR}/src/fdc/wd279x.o \
	${OBJECTDIR}/src/fs_layer.o \
	${OBJECTDIR}/src/gdg/framebuffer.o \
	${OBJECTDIR}/src/gdg/gdg.o \
	${OBJECTDIR}/src/gdg/hwscroll.o \
	${OBJECTDIR}/src/gdg/vramctrl.o \
	${OBJECTDIR}/src/ide8/ide8.o \
	${OBJECTDIR}/src/iface_sdl/iface_sdl.o \
	${OBJECTDIR}/src/iface_sdl/iface_sdl_audio.o \
	${OBJECTDIR}/src/iface_sdl/iface_sdl_joy.o \
	${OBJECTDIR}/src/iface_sdl/iface_sdl_keyboard.o \
	${OBJECTDIR}/src/iface_sdl/iface_sdl_log.o \
	${OBJECTDIR}/src/joy/joy.o \
	${OBJECTDIR}/src/libs/cmt_stream/cmt_bitstream.o \
	${OBJECTDIR}/src/libs/cmt_stream/cmt_stream.o \
	${OBJECTDIR}/src/libs/cmt_stream/cmt_vstream.o \
	${OBJECTDIR}/src/libs/cmttool/cmttool.o \
	${OBJECTDIR}/src/libs/cmttool/cmttool_fastipl.o \
	${OBJECTDIR}/src/libs/cmttool/cmttool_turbo.o \
	${OBJECTDIR}/src/libs/dsk/dsk.o \
	${OBJECTDIR}/src/libs/dsk/dsk_tools.o \
	${OBJECTDIR}/src/libs/endianity/endianity.o \
	${OBJECTDIR}/src/libs/generic_driver/generic_driver.o \
	${OBJECTDIR}/src/libs/mzf/mzf.o \
	${OBJECTDIR}/src/libs/mzf/mzf_tools.o \
	${OBJECTDIR}/src/libs/mztape/cmtspeed.o \
	${OBJECTDIR}/src/libs/mztape/mztape.o \
	${OBJECTDIR}/src/libs/qd/qd.o \
	${OBJECTDIR}/src/libs/wav/wav.o \
	${OBJECTDIR}/src/libs/zxtape/zxtape.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_MZ700.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ700.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ700.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ700.o \
	${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/ROM_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/ROM_MZ700.o \
	${OBJECTDIR}/src/memory/ROM/ROM_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_MZ700.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_en_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_en_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_ge_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_ge_MZ800.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_jap_CGROM.o \
	${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_jap_MZ800.o \
	${OBJECTDIR}/src/memory/memext.o \
	${OBJECTDIR}/src/memory/memory.o \
	${OBJECTDIR}/src/memory/rom.o \
	${OBJECTDIR}/src/mz800.o \
	${OBJECTDIR}/src/pio8255/pio8255.o \
	${OBJECTDIR}/src/pioz80/pioz80.o \
	${OBJECTDIR}/src/port.o \
	${OBJECTDIR}/src/psg/psg.o \
	${OBJECTDIR}/src/qdisk/qdisk.o \
	${OBJECTDIR}/src/ramdisk/ramdisk.o \
	${OBJECTDIR}/src/sharpmz_ascii.o \
	${OBJECTDIR}/src/ui/debugger/ui_breakpoints.o \
	${OBJECTDIR}/src/ui/debugger/ui_dbg_memext.o \
	${OBJECTDIR}/src/ui/debugger/ui_debugger.o \
	${OBJECTDIR}/src/ui/debugger/ui_debugger_callbacks.o \
	${OBJECTDIR}/src/ui/debugger/ui_debugger_iasm.o \
	${OBJECTDIR}/src/ui/debugger/ui_dissassembler.o \
	${OBJECTDIR}/src/ui/debugger/ui_membrowser.o \
	${OBJECTDIR}/src/ui/debugger/ui_memload.o \
	${OBJECTDIR}/src/ui/debugger/ui_memsave.o \
	${OBJECTDIR}/src/ui/dsk_tool/ui_dsk_tool.o \
	${OBJECTDIR}/src/ui/generic_driver/ui_file_driver.o \
	${OBJECTDIR}/src/ui/generic_driver/ui_memory_driver.o \
	${OBJECTDIR}/src/ui/tools/ui_tool_pixbuf.o \
	${OBJECTDIR}/src/ui/ui_audio.o \
	${OBJECTDIR}/src/ui/ui_cmt.o \
	${OBJECTDIR}/src/ui/ui_cmt_tape.o \
	${OBJECTDIR}/src/ui/ui_display.o \
	${OBJECTDIR}/src/ui/ui_fcbutton.o \
	${OBJECTDIR}/src/ui/ui_fdc.o \
	${OBJECTDIR}/src/ui/ui_file_chooser.o \
	${OBJECTDIR}/src/ui/ui_hexeditable.o \
	${OBJECTDIR}/src/ui/ui_ide8.o \
	${OBJECTDIR}/src/ui/ui_joy.o \
	${OBJECTDIR}/src/ui/ui_main.o \
	${OBJECTDIR}/src/ui/ui_memext.o \
	${OBJECTDIR}/src/ui/ui_qdisk.o \
	${OBJECTDIR}/src/ui/ui_ramdisk.o \
	${OBJECTDIR}/src/ui/ui_rom.o \
	${OBJECTDIR}/src/ui/ui_unicard.o \
	${OBJECTDIR}/src/ui/ui_utils.o \
	${OBJECTDIR}/src/ui/ui_version_check.o \
	${OBJECTDIR}/src/ui/vkbd/ui_vkbd.o \
	${OBJECTDIR}/src/ui/vkbd/ui_vkbd_autotype.o \
	${OBJECTDIR}/src/ui/vkbd/ui_vkbd_linux_x11.o \
	${OBJECTDIR}/src/ui/vkbd/ui_vkbd_windows.o \
	${OBJECTDIR}/src/unicard/MGR_MZF.o \
	${OBJECTDIR}/src/unicard/MZFLOADER_MZQ.o \
	${OBJECTDIR}/src/unicard/unicard.o \
	${OBJECTDIR}/src/unicard/unimgr.o \
	${OBJECTDIR}/src/version_check/version_check.o \
	${OBJECTDIR}/src/version_check/version_xml_parser.o \
	${OBJECTDIR}/src/z80ex/z80ex.o \
	${OBJECTDIR}/src/z80ex/z80ex_dasm.o


# C Compiler Flags
CFLAGS=-pedantic -pipe -Wl,--export-dynamic ${PROJECT_CFLAGS}

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mz800emu

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mz800emu: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	gcc -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mz800emu ${OBJECTFILES} ${LDLIBSOPTIONS} ${PROJECT_LIBS}

${OBJECTDIR}/src/audio.o: src/audio.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/audio.o src/audio.c

${OBJECTDIR}/src/build_time.o: src/build_time.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/build_time.o src/build_time.c

${OBJECTDIR}/src/cfgfile/cfgelement.o: src/cfgfile/cfgelement.c 
	${MKDIR} -p ${OBJECTDIR}/src/cfgfile
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cfgfile/cfgelement.o src/cfgfile/cfgelement.c

${OBJECTDIR}/src/cfgfile/cfgmodule.o: src/cfgfile/cfgmodule.c 
	${MKDIR} -p ${OBJECTDIR}/src/cfgfile
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cfgfile/cfgmodule.o src/cfgfile/cfgmodule.c

${OBJECTDIR}/src/cfgfile/cfgroot.o: src/cfgfile/cfgroot.c 
	${MKDIR} -p ${OBJECTDIR}/src/cfgfile
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cfgfile/cfgroot.o src/cfgfile/cfgroot.c

${OBJECTDIR}/src/cfgfile/cfgtools.o: src/cfgfile/cfgtools.c 
	${MKDIR} -p ${OBJECTDIR}/src/cfgfile
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cfgfile/cfgtools.o src/cfgfile/cfgtools.c

${OBJECTDIR}/src/cfgmain.o: src/cfgmain.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cfgmain.o src/cfgmain.c

${OBJECTDIR}/src/cmt/cmt.o: src/cmt/cmt.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmt.o src/cmt/cmt.c

${OBJECTDIR}/src/cmt/cmt_mzf.o: src/cmt/cmt_mzf.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmt_mzf.o src/cmt/cmt_mzf.c

${OBJECTDIR}/src/cmt/cmt_mzftape.o: src/cmt/cmt_mzftape.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmt_mzftape.o src/cmt/cmt_mzftape.c

${OBJECTDIR}/src/cmt/cmt_save.o: src/cmt/cmt_save.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmt_save.o src/cmt/cmt_save.c

${OBJECTDIR}/src/cmt/cmt_tap.o: src/cmt/cmt_tap.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmt_tap.o src/cmt/cmt_tap.c

${OBJECTDIR}/src/cmt/cmt_wav.o: src/cmt/cmt_wav.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmt_wav.o src/cmt/cmt_wav.c

${OBJECTDIR}/src/cmt/cmtext.o: src/cmt/cmtext.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmtext.o src/cmt/cmtext.c

${OBJECTDIR}/src/cmt/cmtext_block.o: src/cmt/cmtext_block.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmtext_block.o src/cmt/cmtext_block.c

${OBJECTDIR}/src/cmt/cmtext_container.o: src/cmt/cmtext_container.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmtext_container.o src/cmt/cmtext_container.c

${OBJECTDIR}/src/cmt/cmthack.o: src/cmt/cmthack.c 
	${MKDIR} -p ${OBJECTDIR}/src/cmt
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/cmt/cmthack.o src/cmt/cmthack.c

${OBJECTDIR}/src/ctc8253/ctc8253.o: src/ctc8253/ctc8253.c 
	${MKDIR} -p ${OBJECTDIR}/src/ctc8253
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ctc8253/ctc8253.o src/ctc8253/ctc8253.c

${OBJECTDIR}/src/debugger/breakpoints.o: src/debugger/breakpoints.c 
	${MKDIR} -p ${OBJECTDIR}/src/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/debugger/breakpoints.o src/debugger/breakpoints.c

${OBJECTDIR}/src/debugger/debugger.o: src/debugger/debugger.c 
	${MKDIR} -p ${OBJECTDIR}/src/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/debugger/debugger.o src/debugger/debugger.c

${OBJECTDIR}/src/debugger/inline_asm.o: src/debugger/inline_asm.c 
	${MKDIR} -p ${OBJECTDIR}/src/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/debugger/inline_asm.o src/debugger/inline_asm.c

${OBJECTDIR}/src/display.o: src/display.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/display.o src/display.c

${OBJECTDIR}/src/fdc/fdc.o: src/fdc/fdc.c 
	${MKDIR} -p ${OBJECTDIR}/src/fdc
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/fdc/fdc.o src/fdc/fdc.c

${OBJECTDIR}/src/fdc/wd279x.o: src/fdc/wd279x.c 
	${MKDIR} -p ${OBJECTDIR}/src/fdc
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/fdc/wd279x.o src/fdc/wd279x.c

${OBJECTDIR}/src/fs_layer.o: src/fs_layer.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/fs_layer.o src/fs_layer.c

${OBJECTDIR}/src/gdg/framebuffer.o: src/gdg/framebuffer.c 
	${MKDIR} -p ${OBJECTDIR}/src/gdg
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gdg/framebuffer.o src/gdg/framebuffer.c

${OBJECTDIR}/src/gdg/gdg.o: src/gdg/gdg.c 
	${MKDIR} -p ${OBJECTDIR}/src/gdg
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gdg/gdg.o src/gdg/gdg.c

${OBJECTDIR}/src/gdg/hwscroll.o: src/gdg/hwscroll.c 
	${MKDIR} -p ${OBJECTDIR}/src/gdg
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gdg/hwscroll.o src/gdg/hwscroll.c

${OBJECTDIR}/src/gdg/vramctrl.o: src/gdg/vramctrl.c 
	${MKDIR} -p ${OBJECTDIR}/src/gdg
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gdg/vramctrl.o src/gdg/vramctrl.c

${OBJECTDIR}/src/ide8/ide8.o: src/ide8/ide8.c 
	${MKDIR} -p ${OBJECTDIR}/src/ide8
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ide8/ide8.o src/ide8/ide8.c

${OBJECTDIR}/src/iface_sdl/iface_sdl.o: src/iface_sdl/iface_sdl.c 
	${MKDIR} -p ${OBJECTDIR}/src/iface_sdl
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iface_sdl/iface_sdl.o src/iface_sdl/iface_sdl.c

${OBJECTDIR}/src/iface_sdl/iface_sdl_audio.o: src/iface_sdl/iface_sdl_audio.c 
	${MKDIR} -p ${OBJECTDIR}/src/iface_sdl
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iface_sdl/iface_sdl_audio.o src/iface_sdl/iface_sdl_audio.c

${OBJECTDIR}/src/iface_sdl/iface_sdl_joy.o: src/iface_sdl/iface_sdl_joy.c 
	${MKDIR} -p ${OBJECTDIR}/src/iface_sdl
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iface_sdl/iface_sdl_joy.o src/iface_sdl/iface_sdl_joy.c

${OBJECTDIR}/src/iface_sdl/iface_sdl_keyboard.o: src/iface_sdl/iface_sdl_keyboard.c 
	${MKDIR} -p ${OBJECTDIR}/src/iface_sdl
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iface_sdl/iface_sdl_keyboard.o src/iface_sdl/iface_sdl_keyboard.c

${OBJECTDIR}/src/iface_sdl/iface_sdl_log.o: src/iface_sdl/iface_sdl_log.c 
	${MKDIR} -p ${OBJECTDIR}/src/iface_sdl
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iface_sdl/iface_sdl_log.o src/iface_sdl/iface_sdl_log.c

${OBJECTDIR}/src/joy/joy.o: src/joy/joy.c 
	${MKDIR} -p ${OBJECTDIR}/src/joy
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/joy/joy.o src/joy/joy.c

${OBJECTDIR}/src/libs/cmt_stream/cmt_bitstream.o: src/libs/cmt_stream/cmt_bitstream.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/cmt_stream
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/cmt_stream/cmt_bitstream.o src/libs/cmt_stream/cmt_bitstream.c

${OBJECTDIR}/src/libs/cmt_stream/cmt_stream.o: src/libs/cmt_stream/cmt_stream.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/cmt_stream
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/cmt_stream/cmt_stream.o src/libs/cmt_stream/cmt_stream.c

${OBJECTDIR}/src/libs/cmt_stream/cmt_vstream.o: src/libs/cmt_stream/cmt_vstream.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/cmt_stream
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/cmt_stream/cmt_vstream.o src/libs/cmt_stream/cmt_vstream.c

${OBJECTDIR}/src/libs/cmttool/cmttool.o: src/libs/cmttool/cmttool.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/cmttool
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/cmttool/cmttool.o src/libs/cmttool/cmttool.c

${OBJECTDIR}/src/libs/cmttool/cmttool_fastipl.o: src/libs/cmttool/cmttool_fastipl.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/cmttool
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/cmttool/cmttool_fastipl.o src/libs/cmttool/cmttool_fastipl.c

${OBJECTDIR}/src/libs/cmttool/cmttool_turbo.o: src/libs/cmttool/cmttool_turbo.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/cmttool
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/cmttool/cmttool_turbo.o src/libs/cmttool/cmttool_turbo.c

${OBJECTDIR}/src/libs/dsk/dsk.o: src/libs/dsk/dsk.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/dsk
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/dsk/dsk.o src/libs/dsk/dsk.c

${OBJECTDIR}/src/libs/dsk/dsk_tools.o: src/libs/dsk/dsk_tools.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/dsk
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/dsk/dsk_tools.o src/libs/dsk/dsk_tools.c

${OBJECTDIR}/src/libs/endianity/endianity.o: src/libs/endianity/endianity.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/endianity
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/endianity/endianity.o src/libs/endianity/endianity.c

${OBJECTDIR}/src/libs/generic_driver/generic_driver.o: src/libs/generic_driver/generic_driver.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/generic_driver
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/generic_driver/generic_driver.o src/libs/generic_driver/generic_driver.c

${OBJECTDIR}/src/libs/mzf/mzf.o: src/libs/mzf/mzf.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/mzf
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/mzf/mzf.o src/libs/mzf/mzf.c

${OBJECTDIR}/src/libs/mzf/mzf_tools.o: src/libs/mzf/mzf_tools.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/mzf
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/mzf/mzf_tools.o src/libs/mzf/mzf_tools.c

${OBJECTDIR}/src/libs/mztape/cmtspeed.o: src/libs/mztape/cmtspeed.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/mztape
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/mztape/cmtspeed.o src/libs/mztape/cmtspeed.c

${OBJECTDIR}/src/libs/mztape/mztape.o: src/libs/mztape/mztape.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/mztape
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/mztape/mztape.o src/libs/mztape/mztape.c

${OBJECTDIR}/src/libs/qd/qd.o: src/libs/qd/qd.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/qd
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/qd/qd.o src/libs/qd/qd.c

${OBJECTDIR}/src/libs/wav/wav.o: src/libs/wav/wav.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/wav
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/wav/wav.o src/libs/wav/wav.c

${OBJECTDIR}/src/libs/zxtape/zxtape.o: src/libs/zxtape/zxtape.c 
	${MKDIR} -p ${OBJECTDIR}/src/libs/zxtape
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libs/zxtape/zxtape.o src/libs/zxtape/zxtape.c

${OBJECTDIR}/src/main.o: src/main.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.c

${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_CGROM.o: src/memory/ROM/JSS-1.3/ROM_JSS103_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.3
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_CGROM.o src/memory/ROM/JSS-1.3/ROM_JSS103_CGROM.c

${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_MZ700.o: src/memory/ROM/JSS-1.3/ROM_JSS103_MZ700.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.3
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_MZ700.o src/memory/ROM/JSS-1.3/ROM_JSS103_MZ700.c

${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_MZ800.o: src/memory/ROM/JSS-1.3/ROM_JSS103_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.3
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.3/ROM_JSS103_MZ800.o src/memory/ROM/JSS-1.3/ROM_JSS103_MZ800.c

${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_CGROM.o: src/memory/ROM/JSS-1.5C/ROM_JSS105C_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.5C
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_CGROM.o src/memory/ROM/JSS-1.5C/ROM_JSS105C_CGROM.c

${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ700.o: src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ700.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.5C
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ700.o src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ700.c

${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ800.o: src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.5C
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ800.o src/memory/ROM/JSS-1.5C/ROM_JSS105C_MZ800.c

${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_CGROM.o: src/memory/ROM/JSS-1.6A/ROM_JSS106A_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.6A
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_CGROM.o src/memory/ROM/JSS-1.6A/ROM_JSS106A_CGROM.c

${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ700.o: src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ700.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.6A
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ700.o src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ700.c

${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ800.o: src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.6A
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ800.o src/memory/ROM/JSS-1.6A/ROM_JSS106A_MZ800.c

${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_CGROM.o: src/memory/ROM/JSS-1.8C/ROM_JSS108C_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.8C
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_CGROM.o src/memory/ROM/JSS-1.8C/ROM_JSS108C_CGROM.c

${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ700.o: src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ700.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.8C
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ700.o src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ700.c

${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ800.o: src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/JSS-1.8C
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ800.o src/memory/ROM/JSS-1.8C/ROM_JSS108C_MZ800.c

${OBJECTDIR}/src/memory/ROM/ROM_CGROM.o: src/memory/ROM/ROM_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/ROM_CGROM.o src/memory/ROM/ROM_CGROM.c

${OBJECTDIR}/src/memory/ROM/ROM_MZ700.o: src/memory/ROM/ROM_MZ700.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/ROM_MZ700.o src/memory/ROM/ROM_MZ700.c

${OBJECTDIR}/src/memory/ROM/ROM_MZ800.o: src/memory/ROM/ROM_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/ROM_MZ800.o src/memory/ROM/ROM_MZ800.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_MZ700.o: src/memory/ROM/WILLY/ROM_WILLY_MZ700.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_MZ700.o src/memory/ROM/WILLY/ROM_WILLY_MZ700.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_en_CGROM.o: src/memory/ROM/WILLY/ROM_WILLY_en_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_en_CGROM.o src/memory/ROM/WILLY/ROM_WILLY_en_CGROM.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_en_MZ800.o: src/memory/ROM/WILLY/ROM_WILLY_en_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_en_MZ800.o src/memory/ROM/WILLY/ROM_WILLY_en_MZ800.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_ge_CGROM.o: src/memory/ROM/WILLY/ROM_WILLY_ge_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_ge_CGROM.o src/memory/ROM/WILLY/ROM_WILLY_ge_CGROM.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_ge_MZ800.o: src/memory/ROM/WILLY/ROM_WILLY_ge_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_ge_MZ800.o src/memory/ROM/WILLY/ROM_WILLY_ge_MZ800.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_jap_CGROM.o: src/memory/ROM/WILLY/ROM_WILLY_jap_CGROM.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_jap_CGROM.o src/memory/ROM/WILLY/ROM_WILLY_jap_CGROM.c

${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_jap_MZ800.o: src/memory/ROM/WILLY/ROM_WILLY_jap_MZ800.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory/ROM/WILLY
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/ROM/WILLY/ROM_WILLY_jap_MZ800.o src/memory/ROM/WILLY/ROM_WILLY_jap_MZ800.c

${OBJECTDIR}/src/memory/memext.o: src/memory/memext.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/memext.o src/memory/memext.c

${OBJECTDIR}/src/memory/memory.o: src/memory/memory.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/memory.o src/memory/memory.c

${OBJECTDIR}/src/memory/rom.o: src/memory/rom.c 
	${MKDIR} -p ${OBJECTDIR}/src/memory
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/memory/rom.o src/memory/rom.c

${OBJECTDIR}/src/mz800.o: src/mz800.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/mz800.o src/mz800.c

${OBJECTDIR}/src/pio8255/pio8255.o: src/pio8255/pio8255.c 
	${MKDIR} -p ${OBJECTDIR}/src/pio8255
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/pio8255/pio8255.o src/pio8255/pio8255.c

${OBJECTDIR}/src/pioz80/pioz80.o: src/pioz80/pioz80.c 
	${MKDIR} -p ${OBJECTDIR}/src/pioz80
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/pioz80/pioz80.o src/pioz80/pioz80.c

${OBJECTDIR}/src/port.o: src/port.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/port.o src/port.c

${OBJECTDIR}/src/psg/psg.o: src/psg/psg.c 
	${MKDIR} -p ${OBJECTDIR}/src/psg
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/psg/psg.o src/psg/psg.c

${OBJECTDIR}/src/qdisk/qdisk.o: src/qdisk/qdisk.c 
	${MKDIR} -p ${OBJECTDIR}/src/qdisk
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/qdisk/qdisk.o src/qdisk/qdisk.c

${OBJECTDIR}/src/ramdisk/ramdisk.o: src/ramdisk/ramdisk.c 
	${MKDIR} -p ${OBJECTDIR}/src/ramdisk
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ramdisk/ramdisk.o src/ramdisk/ramdisk.c

${OBJECTDIR}/src/sharpmz_ascii.o: src/sharpmz_ascii.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/sharpmz_ascii.o src/sharpmz_ascii.c

${OBJECTDIR}/src/ui/debugger/ui_breakpoints.o: src/ui/debugger/ui_breakpoints.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_breakpoints.o src/ui/debugger/ui_breakpoints.c

${OBJECTDIR}/src/ui/debugger/ui_dbg_memext.o: src/ui/debugger/ui_dbg_memext.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_dbg_memext.o src/ui/debugger/ui_dbg_memext.c

${OBJECTDIR}/src/ui/debugger/ui_debugger.o: src/ui/debugger/ui_debugger.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_debugger.o src/ui/debugger/ui_debugger.c

${OBJECTDIR}/src/ui/debugger/ui_debugger_callbacks.o: src/ui/debugger/ui_debugger_callbacks.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_debugger_callbacks.o src/ui/debugger/ui_debugger_callbacks.c

${OBJECTDIR}/src/ui/debugger/ui_debugger_iasm.o: src/ui/debugger/ui_debugger_iasm.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_debugger_iasm.o src/ui/debugger/ui_debugger_iasm.c

${OBJECTDIR}/src/ui/debugger/ui_dissassembler.o: src/ui/debugger/ui_dissassembler.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_dissassembler.o src/ui/debugger/ui_dissassembler.c

${OBJECTDIR}/src/ui/debugger/ui_membrowser.o: src/ui/debugger/ui_membrowser.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_membrowser.o src/ui/debugger/ui_membrowser.c

${OBJECTDIR}/src/ui/debugger/ui_memload.o: src/ui/debugger/ui_memload.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_memload.o src/ui/debugger/ui_memload.c

${OBJECTDIR}/src/ui/debugger/ui_memsave.o: src/ui/debugger/ui_memsave.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/debugger
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/debugger/ui_memsave.o src/ui/debugger/ui_memsave.c

${OBJECTDIR}/src/ui/dsk_tool/ui_dsk_tool.o: src/ui/dsk_tool/ui_dsk_tool.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/dsk_tool
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/dsk_tool/ui_dsk_tool.o src/ui/dsk_tool/ui_dsk_tool.c

${OBJECTDIR}/src/ui/generic_driver/ui_file_driver.o: src/ui/generic_driver/ui_file_driver.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/generic_driver
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/generic_driver/ui_file_driver.o src/ui/generic_driver/ui_file_driver.c

${OBJECTDIR}/src/ui/generic_driver/ui_memory_driver.o: src/ui/generic_driver/ui_memory_driver.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/generic_driver
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/generic_driver/ui_memory_driver.o src/ui/generic_driver/ui_memory_driver.c

${OBJECTDIR}/src/ui/tools/ui_tool_pixbuf.o: src/ui/tools/ui_tool_pixbuf.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/tools
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/tools/ui_tool_pixbuf.o src/ui/tools/ui_tool_pixbuf.c

${OBJECTDIR}/src/ui/ui_audio.o: src/ui/ui_audio.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_audio.o src/ui/ui_audio.c

${OBJECTDIR}/src/ui/ui_cmt.o: src/ui/ui_cmt.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_cmt.o src/ui/ui_cmt.c

${OBJECTDIR}/src/ui/ui_cmt_tape.o: src/ui/ui_cmt_tape.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_cmt_tape.o src/ui/ui_cmt_tape.c

${OBJECTDIR}/src/ui/ui_display.o: src/ui/ui_display.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_display.o src/ui/ui_display.c

${OBJECTDIR}/src/ui/ui_fcbutton.o: src/ui/ui_fcbutton.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_fcbutton.o src/ui/ui_fcbutton.c

${OBJECTDIR}/src/ui/ui_fdc.o: src/ui/ui_fdc.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_fdc.o src/ui/ui_fdc.c

${OBJECTDIR}/src/ui/ui_file_chooser.o: src/ui/ui_file_chooser.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_file_chooser.o src/ui/ui_file_chooser.c

${OBJECTDIR}/src/ui/ui_hexeditable.o: src/ui/ui_hexeditable.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_hexeditable.o src/ui/ui_hexeditable.c

${OBJECTDIR}/src/ui/ui_ide8.o: src/ui/ui_ide8.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_ide8.o src/ui/ui_ide8.c

${OBJECTDIR}/src/ui/ui_joy.o: src/ui/ui_joy.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_joy.o src/ui/ui_joy.c

${OBJECTDIR}/src/ui/ui_main.o: src/ui/ui_main.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_main.o src/ui/ui_main.c

${OBJECTDIR}/src/ui/ui_memext.o: src/ui/ui_memext.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_memext.o src/ui/ui_memext.c

${OBJECTDIR}/src/ui/ui_qdisk.o: src/ui/ui_qdisk.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_qdisk.o src/ui/ui_qdisk.c

${OBJECTDIR}/src/ui/ui_ramdisk.o: src/ui/ui_ramdisk.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_ramdisk.o src/ui/ui_ramdisk.c

${OBJECTDIR}/src/ui/ui_rom.o: src/ui/ui_rom.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_rom.o src/ui/ui_rom.c

${OBJECTDIR}/src/ui/ui_unicard.o: src/ui/ui_unicard.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_unicard.o src/ui/ui_unicard.c

${OBJECTDIR}/src/ui/ui_utils.o: src/ui/ui_utils.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_utils.o src/ui/ui_utils.c

${OBJECTDIR}/src/ui/ui_version_check.o: src/ui/ui_version_check.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/ui_version_check.o src/ui/ui_version_check.c

${OBJECTDIR}/src/ui/vkbd/ui_vkbd.o: src/ui/vkbd/ui_vkbd.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/vkbd
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/vkbd/ui_vkbd.o src/ui/vkbd/ui_vkbd.c

${OBJECTDIR}/src/ui/vkbd/ui_vkbd_autotype.o: src/ui/vkbd/ui_vkbd_autotype.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/vkbd
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/vkbd/ui_vkbd_autotype.o src/ui/vkbd/ui_vkbd_autotype.c

${OBJECTDIR}/src/ui/vkbd/ui_vkbd_linux_x11.o: src/ui/vkbd/ui_vkbd_linux_x11.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/vkbd
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/vkbd/ui_vkbd_linux_x11.o src/ui/vkbd/ui_vkbd_linux_x11.c

${OBJECTDIR}/src/ui/vkbd/ui_vkbd_windows.o: src/ui/vkbd/ui_vkbd_windows.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui/vkbd
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/vkbd/ui_vkbd_windows.o src/ui/vkbd/ui_vkbd_windows.c

${OBJECTDIR}/src/unicard/MGR_MZF.o: src/unicard/MGR_MZF.c 
	${MKDIR} -p ${OBJECTDIR}/src/unicard
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/unicard/MGR_MZF.o src/unicard/MGR_MZF.c

${OBJECTDIR}/src/unicard/MZFLOADER_MZQ.o: src/unicard/MZFLOADER_MZQ.c 
	${MKDIR} -p ${OBJECTDIR}/src/unicard
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/unicard/MZFLOADER_MZQ.o src/unicard/MZFLOADER_MZQ.c

${OBJECTDIR}/src/unicard/unicard.o: src/unicard/unicard.c 
	${MKDIR} -p ${OBJECTDIR}/src/unicard
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/unicard/unicard.o src/unicard/unicard.c

${OBJECTDIR}/src/unicard/unimgr.o: src/unicard/unimgr.c 
	${MKDIR} -p ${OBJECTDIR}/src/unicard
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/unicard/unimgr.o src/unicard/unimgr.c

${OBJECTDIR}/src/version_check/version_check.o: src/version_check/version_check.c 
	${MKDIR} -p ${OBJECTDIR}/src/version_check
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/version_check/version_check.o src/version_check/version_check.c

${OBJECTDIR}/src/version_check/version_xml_parser.o: src/version_check/version_xml_parser.c 
	${MKDIR} -p ${OBJECTDIR}/src/version_check
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/version_check/version_xml_parser.o src/version_check/version_xml_parser.c

${OBJECTDIR}/src/z80ex/z80ex.o: src/z80ex/z80ex.c 
	${MKDIR} -p ${OBJECTDIR}/src/z80ex
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/z80ex/z80ex.o src/z80ex/z80ex.c

${OBJECTDIR}/src/z80ex/z80ex_dasm.o: src/z80ex/z80ex_dasm.c 
	${MKDIR} -p ${OBJECTDIR}/src/z80ex
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DLINUX -D_REENTRANT -D_XOPEN_SOURCE=500 -DGTK_3_22_30 -I. -Isrc -Isrc/z80ex/ -Isrc/z80ex/include -std=c11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/z80ex/z80ex_dasm.o src/z80ex/z80ex_dasm.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mz800emu

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
