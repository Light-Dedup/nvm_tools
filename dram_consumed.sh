set -e

if [ ! $1 ]; then
	echo Usage: $0 size
	exit
fi

cd ..
make EXTRA_CFLAGS=-DMEASURE_DRAM_USAGE
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh
cd -

sudo fio -directory=/mnt/pmem -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=$1 -name=randrw --dedupe_percentage=0 -group_reporting

sudo dmesg | grep 'Static DRAM usage:' | tail -n 1 | cut -d " " -f 2-

function calc_active_total {
	echo $(($2 * $4)) $(($3 * $4))
}
function print_active_total {
	echo -n "$1 "
	calc_active_total $(sudo grep $1 /proc/slabinfo)
}

echo name active total
print_active_total pmfs_fp_cache
print_active_total pmfs_inner_cache
print_active_total pmfs_kbuf_cache

