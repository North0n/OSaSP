#!/bin/bash

if [ $# -eq 3 ]
then
    IS_CORRECT=true
    if [ ! -w "$1" ]
    then
        IS_CORRECT=false
        echo "Файл не существует или не доступен для записи" >&2
    fi
    
    if [ ! -d "$2" ]
    then
        IS_CORRECT=false
        echo "Заданная директория не существует или не является директорией" >&2
    fi
    
    if [ "$IS_CORRECT" = "true" ]
    then
	find "$2" -maxdepth 1 -name "*.$3" -exec basename {} \; | sort>"$1"
    fi
else
    echo "Введите 3 параметра:"
    echo "  1 - файл, в который будет записан вывод"
    echo "  2 - имя дериктории"
    echo "  3 - расширение файла"
fi
