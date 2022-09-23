#!/usr/bin/bash

set -e

FILE_SYSTEMS=( "Light-Dedup" )
SETUPS=( "setup_nova.sh"  )
BRANCHES=( "master"  )
JOBS=( 16 )

echo "Please run in su mode ..."

for job in "${JOBS[@]}"; do
    STEP=0
    for file_system in "${FILE_SYSTEMS[@]}"; do
        SETUP=${SETUPS[$STEP]}
        
        echo "Start to test $file_system in $job thread ..."
        
        bash "$SETUP" "${BRANCHES[$STEP]}" 0
        mkdir -p /mnt/tmp-"$file_system"/
        ./replay -f /mnt/sdb/FIU_Traces/webmail+online.cs.fiu.edu-110108-113008.1-21.blkparse -d /mnt/pmem0/ -o rw -g null -t "$job" -c 512 -r /mnt/tmp-"$file_system"/
        
        bash "$SETUP" "original" 0
        mkdir -p /mnt/tmp-original/
        ./replay -f /mnt/sdb/FIU_Traces/webmail+online.cs.fiu.edu-110108-113008.1-21.blkparse -d /mnt/pmem0/ -o rw -g null -t "$job" -c 512 -r /mnt/tmp-original/
        
        echo "  Start checking reading content!"
        read_pass=1
        # Make sure path ends with /
        for file in /mnt/tmp-"$file_system"/*; do
            if ! diff -r "$file" /mnt/tmp-original/"$(basename "$file")"; then
                echo "  $file is not correct!"
                read_pass=0
            fi 
        done
        if (( read_pass == 1 )); then
            echo "  Write pass!"
        else 
            echo "  Write fail!"
            exit 1
        fi
        STEP=$((STEP + 1))
    done
done