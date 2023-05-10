#!/bin/sh
set -e
if [ ! $1 ]; then
	echo Usage: $0 pmem-num [timing]
	exit 1
fi

if [ ! $2 ]; then
    timing=0
else
    timing=$1
fi
make -j$(nproc)
echo umounting...
sudo umount /mnt/pmem$1 || true
echo Removing the old kernel module...
sudo rmmod nova || true
echo Inserting the new kernel module...
sudo insmod nova.ko measure_timing=$timing

sleep 1

echo mounting...
sudo mount -t NOVA -o init -o data_cow /dev/pmem$1 /mnt/pmem$1
