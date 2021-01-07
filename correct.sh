set -e
if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size duplicate_percentage
	exit
fi
cd ..
sudo bash setup.sh
cd - > /dev/null

TESTDIR=$HOME/fs_test/test_$(echo $* | sed 's/ /_/g')
if [ ! -d $(realpath $TESTDIR) ]; then
	mkdir -p $TESTDIR
fi
if [ ! -f $TESTDIR/complete ]; then
	sudo bash init_ext4.sh /dev/pmem0
	if [[ $(echo "$(echo $2 | ./toG) * $1 < $(df -h | grep /mnt/pmem | awk '{print $4}' | ./toG) * 0.8" | bc -l) != 0 ]]; then
		sudo rm -r /mnt/pmem/*
		sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=$3 -group_reporting
		cp /mnt/pmem/* $TESTDIR
	else
		fio -directory=$TESTDIR -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=$3 -group_reporting
	fi
	touch $TESTDIR/complete
fi

bash timing_concurrent.sh $*
for file in $(ls /mnt/pmem); do
	echo cmp $TESTDIR/$file /mnt/pmem/$file
	cmp $TESTDIR/$file /mnt/pmem/$file
done

