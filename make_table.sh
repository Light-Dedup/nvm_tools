set -e

if [ ! $3 ]; then
	echo Usage: $0 max_threads available_size\(MiB\) step_of_dup_rate
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
echo dup_rate numjobs each\(MiB\) throughput\(MiB/s\)
for ((dup=0;dup<100;dup=dup+$3)); do
	for ((i=1;i<=$1;i=i*2)); do
		each=$(($2 / $i))
		echo -n "$dup $i $each "
		bash timing_concurrent.sh $i ${each}M $dup | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | ./to_MiB_s
		echo
	done
done

