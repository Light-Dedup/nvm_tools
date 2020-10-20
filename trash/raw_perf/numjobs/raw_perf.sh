if [ ! $2 ]; then
	echo Usage: $0 threads path_to_init_shell_script
	exit
fi

cd $(dirname $2)
sudo bash $2

cd /mnt/pmem
# Warm up
sudo fio -filename=./test1 -direct=1 -iodepth 1 -rw=write -ioengine=mmap -bs=256M -thread -numjobs=$1 -size=$((256*$1))M -name=raw_write --dedupe_percentage=0 -group_reporting &> /dev/null

sudo fio -filename=./test1 -direct=1 -iodepth 1 -rw=write -ioengine=mmap -bs=256M -thread -numjobs=$1 -size=$((256*$1))M -name=raw_write --dedupe_percentage=0 -group_reporting
sudo fio -filename=./test1 -direct=1 -iodepth 1 -rw=read -ioengine=mmap -bs=256M -thread -numjobs=$1 -size=$((256*$1))M -name=raw_read --dedupe_percentage=0 -group_reporting

