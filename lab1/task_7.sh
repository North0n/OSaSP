#!/bin/bash

C_FILE="$1"
EXE_FILE="$2"

if [ $# -eq 2 ]; then
	if [ -f "$C_FILE" ]; then
		gcc "$C_FILE" -o "$EXE_FILE" && ./"$EXE_FILE"
	else
		echo "File $C_FILE doesn't exist">&2
	fi
else
	{
		echo "You should enter 2 parameters"
		echo "	First - C code file"
		echo "	Second - name for executable file"
	}>&2
fi
