set -e
if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size duplicate_percentage [block_size]
	exit
fi
if [ $4 ]; then
	bs=$4
else
	bs=4K
fi

TESTDIR=$HOME/fs_test/test_$(echo $* | sed 's/ /_/g')
if !(cd $TESTDIR 2> /dev/null); then
	mkdir -p $TESTDIR
fi
if [ ! -f $TESTDIR/complete ]; then
	write_amount="$(echo $2 | ./toG) * $1"
    sudo bash init_ext4.sh /dev/pmem0
	pmem_volumn="$(df -h | grep /mnt/pmem | awk '{print $4}' | ./toG)"
	if [[ $1 > 1 && $(echo "$write_amount < $pmem_volumn * 0.8" | bc -l) != 0 ]]; then
		sudo rm -r /mnt/pmem/*
		sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=$bs -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=$3 -group_reporting
		cp /mnt/pmem/* $TESTDIR
	else
		fio -directory=$TESTDIR -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=$bs -thread -numjobs=$1 -size=$2 -name=test --dedupe_percentage=$3 -group_reporting
	fi
	touch $TESTDIR/complete
fi
