#!/bin/sh

#
# EXE definitions
#
MKDIR_EXE=mkdir
CP_EXE=cp
FIND_EXE=find
BASENAME_EXE=basename
DIRNAME_EXE=dirname


#for exe_tool in ${SED_EXE} ${WC_EXE} ${EGREP_EXE}; do
#	if [ ! -x ${exe_tool} ]; then
#		echo "ERROR: tool not found '${exe_tool}' !"
#		exit 1
#	fi
#done


DIST_DIR=${1}


if [ ! -e ${DIST_DIR} ]; then
	echo "ERROR: ${0} - DIST_DIR not exist '${DIST_DIR}'!"
	exit 1
fi

checkReturnCode () {
    rc=$?
    if [ $rc != 0 ]
    then
        exit $rc
    fi
}


if [ ! -e ${DIST_DIR}/ui_resources ]; then
	echo "Making copy ui_resources into '${DIST_DIR}' ..."
	${MKDIR_EXE} ${DIST_DIR}/ui_resources
	checkReturnCode
else
	echo "Checking ui_resources in '${DIST_DIR}' ..."
fi

PRINT_NEWLINE=""

${FIND_EXE} ./ui_resources/ -type d |\
( while read src_dir;do
    if [ ! -e ${DIST_DIR}/${src_dir} ]; then
        echo -e "\nMKDIR ${DIST_DIR}/${src_dir}\c"
        ${MKDIR_EXE} -p ${DIST_DIR}/${src_dir}
	checkReturnCode
	PRINT_NEWLINE="\n"
    fi
done && echo -e $PRINT_NEWLINE )


PRINT_NEWLINE=""

${FIND_EXE} ./ui_resources/ -type f -name "*.glade" -or -name "*.css" -or -name "*.png" -or -name "*.bmp" -or -name "*.gif" |\
( while read src_file;do
	filename=`${BASENAME_EXE} ${src_file}`
	dst_file=${DIST_DIR}/${src_file}
	if [ ! -e ${dst_file} ] || [ ${src_file} -nt ${dst_file} ]; then
                destdir=`${DIRNAME_EXE} ${src_file}`
		echo -e "\nCOPY ${filename} into ${DIST_DIR}/${destdir}\c"
		${CP_EXE} "${src_file}" "${dst_file}"
		checkReturnCode
		PRINT_NEWLINE="\n"
	fi
done && echo -e $PRINT_NEWLINE )

exit 0
