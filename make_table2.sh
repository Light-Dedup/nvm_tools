set -e

if [ ! $2 ]; then
	echo Usage: $0 max_threads available_size\(MiB\)
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
for i in $(seq 1 $1); do
	each=$(($2 / $i))
	echo -n "$i $each "
	bash timing_concurrent.sh $i ${each}M 0 > /dev/null
	sudo mkdir /mnt/pmem/0/
	sudo mv /mnt/pmem/* /mnt/pmem/0/ &> /dev/null || true
	bash timing_concurrent.sh $i ${each}M 0 | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s
	echo
done