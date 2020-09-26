# $1: Number of threads

set -e

if [ ! $2 ]; then
	echo Usage: $0 num_of_threads size
	exit
fi
cd ..
make
sudo bash setup-pmfs.sh
cd /mnt/pmem

TMPFILE=$(mktemp)
for i in $(seq 1 $1); do
	sudo fio -filename=./test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=4K -runtime=10 -thread -numjobs=1 -size=$2 -name=randrw --dedupe_percentage=80 -group_reporting >> $TMPFILE &
done

wait
cd -
make
TMPOUT=$(mktemp)
grep WRITE: $TMPFILE > $TMPOUT
cat $TMPOUT
echo -n Total:
cat $TMPOUT | sed 's/WRITE: bw=//g' | sed 's/MiB.*//g' | ./get_sum
echo MiB/s

rm $TMPFILE $TMPOUT

sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
sudo dmesg | tail -n 50

