#!/bin/bash

DIRECTORY="$1"
FILENAME="$2"

if [ $# -eq 2 ]; then
	if [ -d "$DIRECTORY" ]; then
		echo -n > "$FILENAME"
		OIFS=$IFS
		IFS=$'\n'
		for DIR in $(find "$DIRECTORY" -type d); do
			REALPATH=$(realpath "$DIR")
			SIZE=$(du -hs "$DIR" --apparent-size | cut -f 1)
			COUNT=$(find "$DIR" ! -type d | wc -l)
			printf "%100s %8s %6d\n" "$REALPATH" "$SIZE" "$COUNT" >> "$FILENAME"
		done
		IFS=$OIFS
	else
		echo "$DIRECTORY is not a directory">&2
	fi
else
	{
		echo "You should enter 2 parameters"
		echo "	First - directory's name"
		echo "	Second - output file's name"
	}>&2
fi
