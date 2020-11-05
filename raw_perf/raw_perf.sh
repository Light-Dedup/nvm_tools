set -e

if [ ! $5 ]; then
	echo Usage: $0 path_to_fio path_to_init_scirpt num_of_threads size read_rate
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash $2

sudo $1 -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=rw -ioengine=mmap -bs=4K -thread -numjobs=$3 -size=$4 -name=rw --rwmixread=$5 --dedupe_percentage=0 -group_reporting

