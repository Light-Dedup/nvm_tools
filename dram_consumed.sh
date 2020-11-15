set -e

cd ..
make EXTRA_CFLAGS=-DMEASURE_DRAM_USAGE 1>&2
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh 1>&2
cd - > /dev/null

make 1>&2

sudo dmesg | grep 'Static DRAM usage:' | tail -n 1 | cut -d "]" -f 2 | sed "s/^\s*//g"
echo size\(GiB\) pmfs_fp_cache_active pmfs_fp_cache_total pmfs_inner_cache0_active pmfs_inner_cache0_total pmfs_inner_cache1_active pmfs_inner_cache1_total pmfs_inner_cache2_active pmfs_inner_cache2_total
i=1
while sudo ./write_1G $i 1>&2; do
	echo -n "$i "
	bash print_dram_usage.sh
	echo
	i=$[$i+1]
done

