set -e
if [ ! $4 ]; then
	echo Usage: $0 num_of_threads block_size size duplicate_percentage
	exit
fi
if [ -r pmem/test1 ]; then
	echo Please remount /mnt/pmem
	exit
fi
TESTDIR=/home/searchstar/test
rm -f $TESTDIR/test*
TMPFILE=$(mktemp)
for i in $(seq 1 $1); do
	fio -filename=$TESTDIR/test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=$2 -thread -numjobs=1 -size=$3 -name=randrw --dedupe_percentage=$4 -group_reporting >> $TMPFILE &
done

wait

if [ ! -r get_sum ]; then
	sudo g++ get_sum.cpp -o get_sum
fi
TMPOUT=$(mktemp)
grep WRITE: $TMPFILE > $TMPOUT
cat $TMPOUT
echo -n Total: 
cat $TMPOUT | sed 's/WRITE: bw=//g' | sed 's/MiB.*//g' | ./get_sum
echo MiB/s

rm $TMPFILE $TMPOUT
sudo cp $TESTDIR/test* ./pmem/
for i in $(seq 1 $1); do
	cmp $TESTDIR/test$i ./pmem/test$i
done

