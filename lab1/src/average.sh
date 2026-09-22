#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Ошибка: аргументы не переданы."
    exit 1
fi

sum=0
count=$#

for num in "$@"; do
    sum=$((sum + num))
done
 
average=$(awk "BEGIN {print $sum / $count}")

echo "Количество аргументов: $count"
echo "Среднее арифметическое: $average"

