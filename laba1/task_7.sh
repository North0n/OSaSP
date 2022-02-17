#!/bin/bash

C_FILE="$1"
EXE_FILE="$2"

if [ $# -eq 2 ]; then
	if [ -f "$C_FILE" ]; then
		if gcc "$1" -o "$2"; then
			./"$2"
		fi
	else
		echo "File $C_FILE doesn't exist"
	fi
else
	echo "You should enter 2 parameters"
	echo "	First - C code file"
	echo "	Second - name for executable file"
fi
