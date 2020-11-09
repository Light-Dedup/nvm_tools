set -e

if [ ! $2 ]; then
	echo Usage: $0 num_of_threads size
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
cd ../..
sudo bash setup-pmfs.sh

sudo mkdir /mnt/pmem/pre
sudo fio -directory=/mnt/pmem/pre -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=0 -group_reporting > /dev/null
sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=0 -group_reporting

