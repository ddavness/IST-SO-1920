#!/bin/bash

inputdir=$1
outputdir=$2
maxthreads=$3
buckets=$4

# Create a folder, ignore all errors
mkdir $outputdir 2> /dev/null

function testfile {
    # Push stdout to a black hole, but keep stderr
    $1 > /dev/null 2> /tmp/tecnicofs_stderr.out

    regex="TecnicoFS completed in [0-9]+\.[0-9]+ seconds\."
    lastline=$(tail -2 /tmp/tecnicofs_stderr.out)

    if [[ $lastline =~ $regex ]]; then
        # Execution successful, we can use the trimmed version
        echo $lastline
    else
        # Execution failed, dump all stderr
        cat /tmp/tecnicofs_stderr.out
    fi
}

# This regex expression covers pretty much all POSIX-compliant filenames
# Used to strip off extension names
fregex="([_0-9A-Za-z\-]+)\.([_0-9A-Za-z\-\.]+)?"
function strip {
    [[ $input =~ $fregex ]];
}

for input in $(ls ${1})
do
    strip $inupt
    inputname=${BASH_REMATCH[1]}

    echo "InputFile=${input} NumThreads=1"
    testfile "./tecnicofs-nosync ${inputdir}/${input} ${outputdir}/${inputname}-1.txt 1 1"

    for threads in $(seq 2 ${maxthreads})
    do
        echo "InputFile=${input} NumThreads=${threads}"
        testfile "./tecnicofs-mutex ${inputdir}/${input} ${outputdir}/${inputname}-${threads}.txt ${threads} ${buckets}"
    done

done