#!/bin/bash

OUTPUT_FILE="$1"
DIRECTORY="$2"
FILE_EXT="$3"

if [ $# -eq 3 ]
then
    IS_CORRECT=true    
    if [ ! -d "$DIRECTORY" ]
    then
        IS_CORRECT=false
        echo "Заданная директория не существует или не является директорией" >&2
    fi
    
    if [ "$IS_CORRECT" = "true" ]
    then
		find "$DIRECTORY" -maxdepth 1 -name "*.$FILE_EXT" -exec basename {} \; | sort>"$OUTPUT_FILE"
    fi
else
	{
    	echo "Введите 3 параметра:"
	    echo "  1 - файл, в который будет записан вывод"
	    echo "  2 - имя дериктории"
	    echo "  3 - расширение файла"
    }>&2
fi
