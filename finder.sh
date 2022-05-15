#!/bin/bash

arg=$# #Number of arguments

if [ "$arg" == 0 ]
    then echo Wrong Input, please give at least 1 TLD
    exit -1
fi


for TLD in $*
do
    counter=0
    for file in /tmp/* #/tmp/* is the directory that workers write the .out files
    do
        typeFile=${file: -4} # We keep the 4 last characters of the name of the file
        if [ $typeFile == ".out" ] # We check if the file is of the format <filename>.out 
            then
            # This is because if grep .$TLD[[:space:]] $file is empty then urlSum is not a number and hence cant add counter=$counter+$urlSum
            grepTest=`grep -c .$TLD[[:space:]] $file` 
            if [ $grepTest == 0 ]
            then
                continue
            else
                urlSum=`grep .$TLD[[:space:]] $file | awk '{SUM+=$2}END{print SUM}'` # [[:space:]] in order to not have .com.br as .com 
                let counter=$counter+$urlSum
            fi
        fi
    done
    echo $TLD is the TLD of $counter urls    
done