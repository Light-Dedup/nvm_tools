set -e

cd ..
make EXTRA_CFLAGS=-DMEASURE_DRAM_USAGE
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh
cd -

make
sudo ./exhaust_nvmm

sudo dmesg | grep 'Static DRAM usage:' | tail -n 1 | cut -d "]" -f 2 | sed "s/^\s*//g"

function calc_active_total {
	echo $(($2 * $4)) $(($3 * $4))
}
function print_active_total {
	echo -n "$1 "
	calc_active_total $(sudo grep $1 /proc/slabinfo)
}

echo name active total
# Will reduce by the time.
#print_active_total pmfs_kbuf_cache
print_active_total pmfs_fp_cache
print_active_total pmfs_inner_cache0
print_active_total pmfs_inner_cache1
print_active_total pmfs_inner_cache2

cd ..
sudo ./ioctl_test
sudo ./ioctl_table_stat
sudo dmesg | tail -n 50
cd -
