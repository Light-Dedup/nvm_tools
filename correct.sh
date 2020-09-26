set -e
if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size duplicate_percentage
	exit
fi
TESTDIR=/home/searchstar/test
rm $TESTDIR/test*
TMPFILE=$(mktemp)
for i in $(seq 1 $1); do
	fio -filename=$TESTDIR/test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=4K -runtime=10 -thread -numjobs=1 -size=$2 -name=randrw --dedupe_percentage=$3 -group_reporting >> $TMPFILE &
done

wait

if [ ! -r get_sum ]; then
	g++ get_sum.cpp -o get_sum
fi
TMPOUT=$(mktemp)
grep WRITE: $TMPFILE > $TMPOUT
cat $TMPOUT
echo -n Total: 
cat $TMPOUT | sed 's/WRITE: bw=//g' | sed 's/MiB.*//g' | ./get_sum
echo MiB/s

rm $TMPFILE $TMPOUT
for i in $(seq 1 $1); do
	cmp $TESTDIR/test$i ./pmem/test$i
done

