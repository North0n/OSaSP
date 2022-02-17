#!/bin/bash

DIRECTORY="$1"
FILENAME="$2"

findDirSize(){
	# $1 - passed dir
	local SIZE=0
	echo "$(find "$1" -type d)"
	for DIR in $(find "$1" -type d); do
		echo "$(pwd)"
		echo "$(realpath "$1")"
		if [ ! "$(pwd)" = "$(realpath "$1")" ]; then
			echo "2"
			SIZE=$(($SIZE+$(findDirSize "$DIR")))
		fi
		echo "1"
	done
	for FILE in $(find "$1" -type f); do
		let SIZE=$SIZE+$(du -h "$FILE" --apparent-size)
	done
	PATH=$(realpath "$1")
	printf "%60s%6d\n" "$PATH" "$SIZE" >>"$FILENAME"
	echo "$SIZE"
}

echo "$(realpath "$1")"

if [ $# -eq 2 ]; then
	# find DIRECTORY -print выводит полные имена файлов
	# du -h <filename> --apparent-size выводит размер файла
	echo -n>$FILENAME
	findDirSize "$DIRECTORY"
else
	echo "You should enter 2 parameters"
	echo "	First - directory's name"
	echo "	Second - output file's name"
fi
