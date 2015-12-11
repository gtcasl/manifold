#!/bin/sh

# Use at your own risk!

if [ $# -ne 3 ]
then
    echo "Usage: $0 <old basename> <new basename> <K>"
    echo "Change <old>0, ... <old>K, to <new>0, ... <new>K"
    echo "For example, to rename a0, a1 to b0, b1:  rename.sh a b 1"
    exit 1
fi

for i in `seq 0 $3`
do
    if [ -e $1$i ]
    then
        mv $1$i $2$i
    else
        echo "$1$i does not exist!"
    fi
done
