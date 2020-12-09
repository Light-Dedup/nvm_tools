set -e

if [ ! $1 ]; then
	echo Usage: $0 mx\(GiB\)
	exit
fi

cd ..
make EXTRA_CFLAGS=-DMEASURE_DRAM_USAGE 1>&2
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh 1>&2
cd - > /dev/null

make 1>&2

sudo dmesg | grep 'Static DRAM usage:' | tail -n 1 | cut -d "]" -f 2 | sed "s/^\s*//g"
bash ../helper/print_dram_usage_header.sh
mx=3
for i in $(seq 1 $1); do
	sudo ./write_1G $i 1>&2
done

#cd ..
#sudo ./ioctl_table_stat
#cd -

for i in $(seq 1 $1); do
	sudo rm /mnt/pmem/test$i
	echo -n "$i "
	# The information is not precise. The real value reveals after a while.
	bash ../helper/print_dram_usage.sh
	echo
done

#cd ..
#sudo ./ioctl_table_stat
#cd -

