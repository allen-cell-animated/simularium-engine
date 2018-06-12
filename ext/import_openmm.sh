#!/bin/sh

TMP_FILE="opmm.tmp"
TARGET_DIR="openmm"
LIB_DIR="../lib"

# This script imports the needed OpenMM Headers & libraries
#  from an OpenMM Directory that has already been built
while getopts "d:" option; do
	case "${option}" in
		d)
			OPENMM_DIR="$OPTARG"
			;;
		\?)
		       	echo "Invalid option: -${OPTARG}"
			exit 1
			;;
	esac
done

if [ -z ${OPENMM_DIR} ]; then
	echo "ERROR: no openMM directory specified with -d"
	exit 1
fi

rm -rf $TARGET_DIR
mkdir -p $TARGET_DIR

find $OPENMM_DIR -wholename "*/include/*.h" > $TMP_FILE

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

mv $TARGET_DIR/OpenMM.h OpenMM.h

find $OPENMM_DIR -name "*.so" -exec cp {} $LIB_DIR \;
find $OPENMM_DIR -name "*.a" -exec cp {} $LIB_DIR \;
