#!/usr/bin/bash

SIZE=128    # G

sudo bash ./setup_nova.sh master 0

BW=$(sudo fio -filename=/mnt/pmem0/test -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size="${SIZE}"G -name=test --dedupe_percentage=0 -group_reporting | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s)

echo 1 > /proc/fs/NOVA/pmem0/timing_stats

sudo ./shuffle_and_write -f /mnt/pmem0/test -g 4096

BW1=$(sudo fio -filename=/mnt/pmem0/test -fallocate=none -direct=1 -iodepth 1 -rw=read -ioengine=sync -bs=4K -thread -numjobs=1 -size="${SIZE}"G -name=test --dedupe_percentage=0 -group_reporting | grep READ: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s)

BW2=$(sudo fio -filename=/mnt/pmem0/test-shuffled -fallocate=none -direct=1 -iodepth 1 -rw=read -ioengine=sync -bs=4K -thread -numjobs=1 -size="${SIZE}"G -name=test --dedupe_percentage=0 -group_reporting | grep READ: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s)

echo "WriteFirst: $BW MiB/s"
echo "ReadFirst: $BW1 MiB/s"
echo "ReadSecond: $BW2 MiB/s"