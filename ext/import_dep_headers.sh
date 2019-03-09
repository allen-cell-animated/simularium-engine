#!/bin/sh

# Parameter 0 : Project Name
# Parameter 1 : Include Directory

if [ $# -ne 2 ]; then
	echo "Expected two parameters, project_name include_directory"
	exit 0
fi

TMP_FILE="$1.tmp"
TARGET_DIR="$1"
PROJECT_DIRECTORY="../dep/$1"
INCLUDE_DIRECTORY="$2"

rm -rf $TARGET_DIR
mkdir -p $TARGET_DIR

find $PROJECT_DIRECTORY -wholename "*/$INCLUDE_DIRECTORY/*.h" > $TMP_FILE

while read line; do
	fpath="*/$INCLUDE_DIRECTORY/"
	fpath=${line##$fpath}
	fdir="/*.h"
	fdir=${fpath%$fdir}
	if [[ ${fdir} != *".h"* ]]; then
		mkdir -p $TARGET_DIR/$fdir
	fi
	cp  $line $TARGET_DIR/$fpath
done < $TMP_FILE
rm $TMP_FILE

#find $PROJECT_DIRECTORY -wholename "*/$INCLUDE_DIRECTORY/*.cc" > $TMP_FILE

while read line; do
	fpath="*/$INCLUDE_DIRECTORY/"
	fpath=${line##$fpath}
	fdir="/*.cc"
	fdir=${fpath%$fdir}
	if [[ ${fdir} != *".cc"* ]]; then
		mkdir -p $TARGET_DIR/$fdir
	fi
	cp  $line $TARGET_DIR/$fpath
done < $TMP_FILE
rm $TMP_FILE

find $PROJECT_DIRECTORY -wholename "*/$INCLUDE_DIRECTORY/*.hpp" > $TMP_FILE

while read line; do
	fpath="*/$INCLUDE_DIRECTORY/"
	fpath=${line##$fpath}
	fdir="/*.hpp"
	fdir=${fpath%$fdir}
	if [[ ${fdir} != *".hpp"* ]]; then
		mkdir -p $TARGET_DIR/$fdir
	fi
	cp  $line $TARGET_DIR/$fpath
done < $TMP_FILE
rm $TMP_FILE
