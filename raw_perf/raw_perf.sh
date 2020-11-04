set -e

if [ ! $4 ]; then
	echo Usage: $0 path_to_init_scirpt num_of_threads size read_rate
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash $1

sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=rw -ioengine=sync -bs=4K -thread -numjobs=$2 -size=$3 -name=rw --rwmixread=$4 --dedupe_percentage=0 -group_reporting

