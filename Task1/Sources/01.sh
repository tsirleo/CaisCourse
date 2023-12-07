#!/bin/sh
cp $1 input.txt
env -i PWD=/home/student $(readlink -f 01) input.txt
rm input.txt
