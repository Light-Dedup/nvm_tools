set -e
if [[ $# < 4 || $# > 5 ]]; then
	echo Usage: $0 pmem-num num_of_threads size duplicate_percentage [block_size]
	exit 1
fi
if [ $5 ]; then
	bs=$5
else
	bs=4K
fi

TESTDIR=$HOME/fs_test/test_$(echo $2 $3 $4 $bs | sed 's/ /_/g')
if !(cd $TESTDIR 2> /dev/null); then
	mkdir -p $TESTDIR
fi
if [ ! -f $TESTDIR/complete ]; then
	write_amount="$(echo $3 | ./toG) * $2"
    sudo bash setup/ext4.sh $1
	pmem_volumn="$(df -h | grep /mnt/pmem$1 | awk '{print $4}' | ./toG)"
	if [[ $2 > 1 && $(echo "$write_amount < $pmem_volumn * 0.8" | bc -l) != 0 ]]; then
		sudo rm -r /mnt/pmem$1/*
		sudo fio -directory=/mnt/pmem$1 -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=$bs -thread -numjobs=$2 -size=$3 -name=test --dedupe_percentage=$4 -group_reporting
		cp /mnt/pmem$1/* $TESTDIR
	else
		fio -directory=$TESTDIR -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=$bs -thread -numjobs=$2 -size=$3 -name=test --dedupe_percentage=$4 -group_reporting
	fi
	touch $TESTDIR/complete
fi
