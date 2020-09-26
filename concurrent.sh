# $1: Number of threads

if [ 0"$1" = "0" ]; then
	echo Please specify the number of fio threads.
	exit
fi
umount /mnt/pmem
mkfs -t ext4 /dev/pmem0
mount -t ext4 /dev/pmem0 /mnt/pmem -o dax
cd /mnt/pmem

TMPFILE=$(mktemp)
for i in $(seq 1 $1); do
	nohup sudo fio -filename=./test$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=4K -runtime=10 -thread -numjobs=1 -size=30M -name=randrw --dedupe_percentage=80 -group_reporting >> $TMPFILE &
done

wait
cd -
g++ get_sum.cpp -o get_sum
TMPOUT=$(mktemp)
grep WRITE: $TMPFILE > $TMPOUT
cat $TMPOUT
echo -n Total: 
cat $TMPOUT | sed 's/WRITE: bw=//g' | sed 's/MiB.*//g' | ./get_sum
echo MiB/s

rm $TMPFILE $TMPOUT

#sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
#sudo dmesg | tail -n 50

