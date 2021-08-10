set -e

if [ ! $3 ]; then
	echo Usage: $0 max_threads available_size\(MiB\) pre_process_path
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
echo Thread Each\(MiB\) First\(MiB/s\) Second\(MiB/s\)
for ((i=1;i<=$1;i=i*2)); do
	each=$(($2 / $i))
	echo -n "$i $each "
	bash $3 1>&2
	bash helper/fio.sh $i ${each}M 0 | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s
	sudo mkdir /mnt/pmem/0/
	sudo mv /mnt/pmem/* /mnt/pmem/0/ &> /dev/null || true
	echo -n " "
	bash helper/fio.sh $i ${each}M 0 | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s
	echo
done