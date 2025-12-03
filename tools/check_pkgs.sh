#!/bin/sh

if [ $# -lt 4 ] || [ "$1" != "exec:" ] || [ "$3" != "packages:" ]; then
	echo -e "\n\n$0 - ERROR: usage check_pkgs.sh 'exec:' <pkg-config-exec> 'packages:' <pkages...>\n\n"
	exit 1
fi


PKG_CONFIG=$2
shift
shift
shift

while [ $# -ge 1 ]; do
	pkg=$1
	
	echo -e "Check package: ${pkg} - \c"
	${PKG_CONFIG} ${pkg}
	if [ $? -eq 0 ]; then
		echo "OK!"
	else
		echo "FAILED!"
		exit 1
	fi;
	shift
done

exit 0
