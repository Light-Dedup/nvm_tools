set -e

if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate
	exit
fi
cd ..
make
sudo bash setup-pmfs.sh
cd -
cd /mnt/pmem

TMPFILE=$(mktemp)
for i in $(seq 1 $1); do
	sudo fio -filename=./test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=4K -thread -numjobs=1 -size=$2 -name=randrw --dedupe_percentage=$3 -group_reporting >> $TMPFILE &
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

#cd ..
#sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
#sudo dmesg | tail -n 50

