set -e

if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size\(GiB\) read_rate
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
cd ../..
sudo bash setup-pmfs.sh

sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=rw -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$(expr $2 \* 10 \* \( 100 - $3\) ) -name=test --rwmixread=$3 --dedupe_percentage=0 -group_reporting

