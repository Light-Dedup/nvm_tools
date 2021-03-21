if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate
	exit 1
fi

sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=$3 -group_reporting
