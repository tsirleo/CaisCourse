#!/bin/sh
cp $1 input.txt
env -i PWD=/home/student gdb -iex "unset env LINES" -iex "unset env COLUMNS" --args $(readlink -f 01) input.txt
rm input.txt
