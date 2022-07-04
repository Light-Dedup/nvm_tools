if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate [block_size]
	exit 1
fi

if [ $4 ]; then
	bs=$4
else
	bs=4K
fi

sudo fio -directory=/mnt/pmem -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=$bs -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=$3 -group_reporting
