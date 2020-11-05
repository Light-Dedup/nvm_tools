set -e
if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size duplicate_percentage
	exit
fi
cd ..
sudo bash setup-pmfs.sh
cd -
TESTDIR=/home/searchstar/test
TEST_ARG=$(mktemp)
echo $* > $TEST_ARG
if diff $TESTDIR/last_arg.txt $TEST_ARG; then
	echo The last test has the same arguments, reuse it.
	rm $TEST_ARG
else
	rm -f $TESTDIR/last_arg.txt $TESTDIR/test*
	TMPFILE=$(mktemp)
	for i in $(seq 1 $1); do
		fio -filename=$TESTDIR/test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=4K -thread -numjobs=1 -size=$2 -name=randrw --dedupe_percentage=$3 -group_reporting
	done
	mv $TEST_ARG $TESTDIR/last_arg.txt
fi

sudo cp $TESTDIR/test* /mnt/pmem/
for i in $(seq 1 $1); do
	cmp $TESTDIR/test$i /mnt/pmem/test$i
done

