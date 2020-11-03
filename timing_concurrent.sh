set -e

if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate
	exit
fi
cd ..
make
sudo bash -c "echo timing_concurrent $* > /dev/kmsg"
sudo bash setup-pmfs.sh
cd -

TMPOUT=$(mktemp)
sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=randrw --dedupe_percentage=$3 -group_reporting | tee $TMPOUT

make
echo -n Total:
grep WRITE: $TMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum | tr -d "\n"
echo GiB/s

rm $TMPOUT

#cd ..
#sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
#sudo dmesg | tail -n 50

