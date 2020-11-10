set -e

cd ..
make EXTRA_CFLAGS=-DMEASURE_DRAM_USAGE
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh
cd -

make
sudo ./exhaust_nvmm

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
