#!/bin/sh

for id in `ipcs -m | awk '{if($6 == 0) print $2}'`
do
    ipcrm -m $id
done
