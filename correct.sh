set -e
if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size duplicate_percentage
	exit
fi
cd ..
sudo bash setup.sh
cd - > /dev/null

TESTDIR=$HOME/test/test_$(echo $* | sed 's/ /_/g')
if [ ! -d $TESTDIR ]; then
	mkdir -p $TESTDIR
fi
cd $TESTDIR
if [ ! -f complete ]; then
	for i in $(seq 1 $1); do
		fio -filename=$TESTDIR/test$i -randseed=$i -direct=1 -iodepth 1 -rw=write -ioengine=psync -bs=4K -thread -numjobs=1 -size=$2 -name=randrw --dedupe_percentage=$3 -group_reporting
	done
	touch complete
fi

sudo cp $TESTDIR/test* /mnt/pmem/
for i in $(seq 1 $1); do
	cmp $TESTDIR/test$i /mnt/pmem/test$i
done

