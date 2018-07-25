#!/bin/sh

TMP_FILE="opmm.tmp"
TARGET_DIR="openmm"
LIB_DIR="../lib"
PLATFORM="linux"

# This script imports the needed OpenMM Headers & libraries
#  from an OpenMM Directory that has already been built
while getopts "d:" option; do
	case "${option}" in
		d)
			OPENMM_PROJECT_DIR="$OPTARG"
			;;
		b)
			OPENMM_BUILD_DIR="$OPTARG"
			;;
		\?)
		       	echo "Invalid option: -${OPTARG}"
			exit 1
			;;
	esac
done

if [ -z ${OPENMM_PROJECT_DIR} ]; then
	echo "ERROR: no openMM project directory specified with -d"
	exit 1
fi

rm -rf $TARGET_DIR
mkdir -p $TARGET_DIR

find $OPENMM_PROJECT_DIR -wholename "*/include/*.h" > $TMP_FILE

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

rm $TARGET_DIR/pthread.h #includes a windows header
rm $TMP_FILE
