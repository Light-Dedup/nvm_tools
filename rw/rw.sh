set -e

if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size read_rate
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
cd ../..
bash setup.sh

sudo fio -directory=/mnt/pmem -fallocate=none -direct=1 -iodepth 1 -rw=rw -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=rw --rwmixread=$3 --dedupe_percentage=0 -group_reporting

