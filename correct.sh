set -e
if [ ! $4 ]; then
	echo Usage: $0 num_of_threads block_size size duplicate_percentage
	exit
fi
cd ..
sudo bash setup-pmfs.sh
cd -
TESTDIR=/home/searchstar/test
TEST_ARG=$(mktemp)
echo $* > $TEST_ARG
if diff $TESTDIR/test_arg.txt $TEST_ARG; then
	echo The last test has the same arguments, reuse it.
	rm $TEST_ARG
else
	rm -f $TESTDIR/test_arg.txt $TESTDIR/test*
	TMPFILE=$(mktemp)
	for i in $(seq 1 $1); do
		fio -filename=$TESTDIR/test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=$2 -thread -numjobs=1 -size=$3 -name=randrw --dedupe_percentage=$4 -group_reporting >> $TMPFILE &
	done
	wait

	mv $TEST_ARG $TESTDIR/test_arg.txt
	make
	TMPOUT=$(mktemp)
	grep WRITE: $TMPFILE > $TMPOUT
	cat $TMPOUT
	echo -n Total: 
	sed 's/.*WRITE: bw=//g' $TMPOUT | sed 's/iB.*//g' | ./toG | ./get_sum | tr -d "\n"
	echo GiB/s
	rm $TMPFILE $TMPOUT
fi

sudo cp $TESTDIR/test* /mnt/pmem/
for i in $(seq 1 $1); do
	cmp $TESTDIR/test$i /mnt/pmem/test$i
done

