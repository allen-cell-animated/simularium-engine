#!/bin/sh

TMP_FILE="readdy.tmp"
TARGET_DIR="readdy"
PLATFORM="linux"
READDY_PROJECT_DIR="../dep/readdy"

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
