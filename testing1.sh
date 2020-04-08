#!/bin/bash

command_args=(
    "-l -a"                          # 0
    "-l -a -L"                       # 1
    "-l -a -L --max-depth=0"         # 2
    "-l -a -L -S --max-depth=0"      # 3
    "-l -a -L --max-depth=1"         # 4
    "-l -a -L -S --max-depth=1"      # 5
    "-l -a -L --max-depth=2"         # 6
    "-l -a -L -S --max-depth=1"      # 7
    "-l -a -L -B 25"                 # 8
    "-l -a -L -S -B 25"              # 9
    "-l -a -B 25"                    # 10
    "-l -a -b"                       # 11
    "-l"
)

directories=(
    ${HOME}/Desktop/SOPE/FEUP-SOPE-Proj
    ${HOME}/Desktop/SOPE
	${HOME}/Desktop/SOPE/FEUP-SOPE-Proj/TestDir
)

for dir in ${directories[*]}
do
    echo "Dir: ${dir}"
    for i in ${!command_args[*]}
    do
        echo -n "Test no. $i: "
        (./simpledu ${command_args[$i]} ${dir} || echo $?) | sort -k2 > /tmp/testeT_simpledu.txt
        (du ${command_args[$i]} ${dir} || echo $?) | sort -k2 > /tmp/testeT_du.txt
        diff -q /tmp/testeT_simpledu.txt /tmp/testeT_du.txt > /dev/null 2>&1 && echo OK || echo FAILED
        
        # Uncomment line below to print what's wrong (in case of test failed)
        # diff -y /tmp/testeT_simpledu.txt /tmp/testeT_du.txt
    done
done
