#!/bin/sh

TMP_FILE="readdy.tmp"
TARGET_DIR="readdy"
LIB_DIR="../lib"
PLATFORM="linux"

# This script imports the needed ReaDDy Headers & libraries
#  from an ReaDDY Directory that has already been built
while getopts "d:b:" option; do
	case "${option}" in
		d)
			READDY_PROJECT_DIR="$OPTARG"
			;;
		b)
			READDY_BUILD_DIR="$OPTARG"
			;;
		\?)
		       	echo "Invalid option: -${OPTARG}"
			exit 1
			;;
	esac
done

if [ -z ${READDY_PROJECT_DIR} ]; then
	echo "ERROR: no ReaDDy project directory specified with -d"
	exit 1
fi

if [ -z ${READDY_BUILD_DIR} ]; then
	echo "ERROR: no ReaDDy build specified with -b"
	exit 1
fi

rm -rf $TARGET_DIR
mkdir -p $TARGET_DIR

find $READDY_PROJECT_DIR -wholename "*/include/*.h" > $TMP_FILE

while read line; do
	fpath="*/include/"
	fpath=${line##$fpath}
	fdir="/*.h"
	fdir=${fpath%$fdir}
	if [[ ${fdir} != *".h"* ]]; then
		mkdir -p $TARGET_DIR/$fdir
	fi
	cp  $line $TARGET_DIR/$fpath
done < $TMP_FILE
rm $TMP_FILE

find $READDY_PROJECT_DIR -wholename "*/include/*.cc" > $TMP_FILE

while read line; do
	fpath="*/include/"
	fpath=${line##$fpath}
	fdir="/*.cc"
	fdir=${fpath%$fdir}
	if [[ ${fdir} != *".cc"* ]]; then
		mkdir -p $TARGET_DIR/$fdir
	fi
	cp  $line $TARGET_DIR/$fpath
done < $TMP_FILE
rm $TMP_FILE

find $READDY_PROJECT_DIR -wholename "*/include/*.hpp" > $TMP_FILE

while read line; do
	fpath="*/include/"
	fpath=${line##$fpath}
	fdir="/*.hpp"
	fdir=${fpath%$fdir}
	if [[ ${fdir} != *".hpp"* ]]; then
		mkdir -p $TARGET_DIR/$fdir
	fi
	cp  $line $TARGET_DIR/$fpath
done < $TMP_FILE
rm $TMP_FILE

find $READDY_BUILD_DIR -name "*.so" -exec cp {} $LIB_DIR/$PLATFORM \;
find $READDY_BUILD_DIR -name "*.a" -exec cp {} $LIB_DIR/$PLATFORM \;
