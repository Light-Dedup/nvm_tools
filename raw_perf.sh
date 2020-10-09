cd /mnt
sudo bash init_ext4.sh
cd pmem

# Warm up
sudo fio -filename=./test1 -direct=1 -iodepth 1 -rw=write -ioengine=mmap -bs=2G -thread -numjobs=1 -size=2G -name=raw_write --dedupe_percentage=0 -group_reporting &> /dev/null

sudo fio -filename=./test1 -direct=1 -iodepth 1 -rw=write -ioengine=mmap -bs=2G -thread -numjobs=1 -size=2G -name=raw_write --dedupe_percentage=0 -group_reporting
sudo fio -filename=./test1 -direct=1 -iodepth 1 -rw=read -ioengine=mmap -bs=2G -thread -numjobs=1 -size=2G -name=raw_read --dedupe_percentage=0 -group_reporting

