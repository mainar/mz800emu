#!/bin/sh

PROJECT_NAME="mz800emu"

if [ "${1}" != "Release-Win32" ]; then
	echo -e "\n\nERROR: not have package rules for '${1}' !\n"
	exit 0;
fi

PACKAGE_PLATFORM="win32"
#PACKAGE_SURFIX="-with_gtk_and_sdl_libs"
PACKAGE_SURFIX=""

CND_CONF="Release-Win32"

echo -e "\n\nCreate package for: ${CND_CONF}"


#
# EXE definitions
#
SED_EXE=sed
WC_EXE=wc
EGREP_EXE=egrep
RAR_EXE=rar
FIND_EXE=find
RM_EXE=rm
MKDIR_EXE=mkdir
CP_EXE=cp
CHMOD_EXE=chmod
UNAME_EXE=uname
SVN_EXE=svn

# Functions
checkReturnCode () {
    rc=$?
    if [ $rc != 0 ]
    then
        exit $rc
    fi
}


makeDirectory () {
# $1 directory path   
# $2 permission (optional)
    ${MKDIR_EXE} -p "$1"
    checkReturnCode
    if [ "$2" != "" ]
    then
      ${CHMOD_EXE} $2 "$1"
      checkReturnCode
    fi
}


copyFileToTmpDir () {
# $1 from-file path
# $2 to-file path  
# $3 permission    
    ${CP_EXE} "$1" "$2"
    checkReturnCode
    if [ "$3" != "" ]
    then
        ${CHMOD_EXE} $3 "$2"
        checkReturnCode
    fi
}


#for exe_tool in ${SED_EXE} ${WC_EXE} ${EGREP_EXE}; do
#	if [ ! -x ${exe_tool} ]; then
#		echo "ERROR: tool not found '${exe_tool}' !"
#		exit 1
#	fi
#done


EMULATOR_VERSION_TAG_DEF=`${EGREP_EXE} "^(\s){0,}#define\s+CFGMAIN_EMULATOR_VERSION_TAG\s+\".*\"" src/cfgmain.h`

if [ $? -ne 0 ]; then
	echo "ERROR: Can't get EMULATOR_VERSION_TAG_DEF !"
	exit 1
fi



let LINES=`echo "${EMULATOR_VERSION_TAG_DEF}" | ${WC_EXE} -l`
checkReturnCode

if [ ${LINES} -gt 1 ]; then
	echo -e "ERROR: definition the  EMULATOR_VERSION_TAG_DEF has multiple lines:\n"
	echo -e "${EMULATOR_VERSION_TAG_DEF}\n"
	exit 1
fi

EMULATOR_VERSION_TAG_TXT=`echo "${EMULATOR_VERSION_TAG_DEF}" | ${SED_EXE} --regexp-extended -e 's/#define\s+CFGMAIN_EMULATOR_VERSION_TAG\s+\"//' -e 's/".*//'`
checkReturnCode
EMULATOR_VERSION_TAG_TXT=`echo ${EMULATOR_VERSION_TAG_TXT} | ${SED_EXE} --regexp-extended -e 's/^\s+//'`
checkReturnCode
EMULATOR_VERSION_TAG_TXT=`echo ${EMULATOR_VERSION_TAG_TXT} | ${SED_EXE} --regexp-extended -e 's/\s/_/g'`
checkReturnCode

if [ -z "${EMULATOR_VERSION_TAG_TXT}" ]; then
	echo "ERROR: definition the EMULATOR_VERSION_TAG is empty !"
	exit 1
fi


EMULATOR_VERSION_FULL_TXT=${EMULATOR_VERSION_TAG_TXT}


#if [ "${EMULATOR_VERSION_TAG_TXT}" = "stable" ]; then

	EMULATOR_VERSION_NUM_DEF=`${EGREP_EXE} "^(\s){0,}#define\s+CFGMAIN_EMULATOR_VERSION_NUM_STRING\s+\".*\"" src/cfgmain.h`

	if [ $? -ne 0 ]; then
		echo "ERROR: Can't get EMULATOR_VERSION_NUM_DEF !"
		exit 1
	fi


	let LINES=`echo "${EMULATOR_VERSION_NUM_DEF}" | ${WC_EXE} -l`
	checkReturnCode

	if [ ${LINES} -gt 1 ]; then
		echo -e "ERROR: definition the  EMULATOR_VERSION_NUM_DEF has multiple lines:\n"
		echo -e "${EMULATOR_VERSION_NUM_DEF}\n"
		exit 1
	fi


	EMULATOR_VERSION_NUM_TXT=`echo "${EMULATOR_VERSION_NUM_DEF}" | ${SED_EXE} --regexp-extended -e 's/#define\s+CFGMAIN_EMULATOR_VERSION_NUM_STRING\s+\"//' -e 's/".*//'`
	checkReturnCode
	EMULATOR_VERSION_NUM_TXT=`echo ${EMULATOR_VERSION_NUM_TXT} | ${SED_EXE} --regexp-extended -e 's/^\s+//'`
	checkReturnCode
	EMULATOR_VERSION_NUM_TXT=`echo ${EMULATOR_VERSION_NUM_TXT} | ${SED_EXE} --regexp-extended -e 's/\s/_/g'`
	checkReturnCode

	if [ -z "${EMULATOR_VERSION_NUM_TXT}" ]; then
		echo "ERROR: definition the EMULATOR_VERSION_NUM is empty !"
		exit 1
	fi
	
	EMULATOR_VERSION_FULL_TXT="${EMULATOR_VERSION_TAG_TXT}-${EMULATOR_VERSION_NUM_TXT}"
#else

	EMULATOR_REVISION_TXT=`${SVN_EXE} info|${EGREP_EXE} "^Revision:"|${SED_EXE} -e 's/Revision://' -e 's/ //g'`
	checkReturnCode
	
	if [ -z "${EMULATOR_REVISION_TXT}" ]; then
		echo "ERROR: definition the EMULATOR_REVISION_TXT is empty !"
		exit 1
	fi
	
	EMULATOR_VERSION_FULL_TXT="${EMULATOR_VERSION_TAG_TXT}-${EMULATOR_VERSION_NUM_TXT}-rev_${EMULATOR_REVISION_TXT}"
#fi


echo -e "Release version: ${EMULATOR_VERSION_FULL_TXT}\n"

#
# Castecne zkopirovano z nbproject/Package-*.bash
#

# Macros
TOP=`pwd`
CND_PLATFORM=i686-w64-mingw32-Linux
CND_DISTDIR=dist   
CND_BUILDDIR=build
NBTMPDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tmp-packaging
TMPDIRNAME=tmp-packaging
OUTPUT_PATH=${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mz800emu-x86.exe
OUTPUT_BASENAME=mz800emu.exe
PACKAGE_TOP_DIR=${PROJECT_NAME}-${PACKAGE_PLATFORM}-${EMULATOR_VERSION_TAG_TXT}/
PACKAGE_ARCHIVE_NAME=${PROJECT_NAME}-${PACKAGE_PLATFORM}-${EMULATOR_VERSION_FULL_TXT}${PACKAGE_SURFIX}

 

# Setup
cd "${TOP}"
checkReturnCode
${MKDIR_EXE} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/package
${RM_EXE} -rf ${NBTMPDIR}
${MKDIR_EXE} -p ${NBTMPDIR}


# Copy files and create directories and links

PACKAGE_DIRS="
	${PACKAGE_TOP_DIR} \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/cursor_keys \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/row0 \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/row1 \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/row2 \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/row3 \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/row4 \
	${PACKAGE_TOP_DIR}/ui_resources/vkbd/row5 \
	${PACKAGE_TOP_DIR}/ui_resources/icons \
	${PACKAGE_TOP_DIR}/ui_resources/icons/debugger \
	${PACKAGE_TOP_DIR}/Documentation \
	${PACKAGE_TOP_DIR}/runtime/sdl-2 \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/glib-2.0/schemas \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/locale/pt_BR/LC_MESSAGES \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/8x8/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/8x8/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/32x32/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/scalable/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/24x24/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/22x22/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/48x48/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/gnome/16x16/mimetypes \
	${PACKAGE_TOP_DIR}/runtime/gtk-3/share/icons/hicolor \
	${PACKAGE_TOP_DIR}/runtime/libsoup-2.4 \
"


#
# mkdirs
#
cd "${TOP}"
for dirname in ${PACKAGE_DIRS}; do
	if [ ! -z ${dirname} ]; then
		#echo "MKDIR: '${NBTMPDIR}/${dirname}'"
		makeDirectory "${NBTMPDIR}/${dirname}"
	fi
done


#
# copyfiles
#
copyFileToTmpDir "${OUTPUT_PATH}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/${OUTPUT_BASENAME}" 0755
copyFileToTmpDir "src/windows_icon/mz800emu.ico" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/mz800emu.ico" 0644


# ui_resources
for filename in mz800emu_cmt.glade  mz800emu.css  mz800emu_debugger.glade  mz800emu.glade dsk_tool.glade membrowser.glade mz800emu_logo.png
do
	copyFileToTmpDir "ui_resources/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/${filename}" 0644
done

# vkbd/cursor_keys
for filename in down.bmp left.bmp right.bmp up.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/cursor_keys/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/cursor_keys/${filename}" 0644
done

# vkbd/row0
for filename in del.bmp f1.bmp f2.bmp f3.bmp f4.bmp f5.bmp inst.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/row0/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/row0/${filename}" 0644
done

# vkbd/row1
for filename in arrow_up.bmp backslash.bmp break.bmp graph.bmp minus.bmp 0.bmp 1.bmp 2.bmp 3.bmp 4.bmp 5.bmp 6.bmp 7.bmp 8.bmp 9.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/row1/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/row1/${filename}" 0644
done

# vkbd/row2
for filename in at.bmp blank.bmp bracketleft.bmp e.bmp i.bmp libra.bmp o.bmp p.bmp q.bmp r.bmp tab.bmp t.bmp u.bmp w.bmp y.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/row2/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/row2/${filename}" 0644
done

# vkbd/row3
for filename in a.bmp bracketright.bmp colon.bmp cr.bmp ctrl.bmp d.bmp f.bmp g.bmp h.bmp j.bmp k.bmp l.bmp s.bmp semicolon.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/row3/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/row3/${filename}" 0644
done

# vkbd/row4
for filename in alpha.bmp b.bmp c.bmp coma.bmp m.bmp n.bmp period.bmp question.bmp shift_l.bmp shift_r.bmp slash.bmp v.bmp x.bmp z.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/row4/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/row4/${filename}" 0644
done

# vkbd/row5
for filename in space.bmp
do
	copyFileToTmpDir "ui_resources/vkbd/row5/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/vkbd/row5/${filename}" 0644
done

# icons/debugger
for filename in Breakpoints24.png Continue24.gif dissassembler24.png memory_browser24.png Pause24.gif Run_To_Cursor.gif run_to_cursor_instruction24.png step_into_instruction24.png StepInto24.gif step_out_instruction24.png StepOut24.gif step_over_instruction24.png StepOver24.gif
do
	copyFileToTmpDir "ui_resources/icons/debugger/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/ui_resources/icons/debugger/${filename}" 0644
done


# Docs
for filename in Changelog_EN.txt Changelog_CZ.txt Readme_CZ.txt Readme_EN.txt Runtime_libs_CZ.txt Runtime_libs_EN.txt
do
	copyFileToTmpDir "Documentation/${filename}" "${NBTMPDIR}/${PACKAGE_TOP_DIR}/Documentation/${filename}" 0644
done


#
# runtime files
#
cd ~/win32_mz800emu_runtime
${FIND_EXE} -type f | while read filename; do
	cd "${TOP}"
	copyFileToTmpDir ~/win32_mz800emu_runtime/${filename} "${NBTMPDIR}/${PACKAGE_TOP_DIR}/${filename}" 0644
	#echo "${filename} ${NBTMPDIR}/${PACKAGE_TOP_DIR}/${filename} 0644"
done


# Generate tar file
cd "${TOP}"
checkReturnCode

#${RM_EXE} -f ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/package/${PACKAGE_ARCHIVE_NAME}.tar
#cd ${NBTMPDIR}
#tar -vcf ../../../../${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/package/${PACKAGE_ARCHIVE_NAME}.tar *

${RM_EXE} -f ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/package/${PACKAGE_ARCHIVE_NAME}.rar
checkReturnCode

cd ${NBTMPDIR}
checkReturnCode

${RAR_EXE} a ../../../../${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/package/${PACKAGE_ARCHIVE_NAME}.rar *
checkReturnCode

#
# Only on my development desktop:
#
DESKTOP_NAME="arrakis.ordoz.com"

FULL_HOSTNAME=`${UNAME_EXE} -n`

if [ "${FULL_HOSTNAME}" = "${DESKTOP_NAME}" ]; then
	echo -e "\n\nCopy ${PACKAGE_ARCHIVE_NAME}.rar to WIN32 share directory...\n"
	cd "${TOP}"
	checkReturnCode
	${CP_EXE} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/package/${PACKAGE_ARCHIVE_NAME}.rar ~/share/
	checkReturnCode
fi


# Cleanup
cd "${TOP}"
checkReturnCode
${RM_EXE} -rf ${NBTMPDIR}
checkReturnCode

exit 0
