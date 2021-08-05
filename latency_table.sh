set -e

if [ ! $3 ]; then
	echo Usage: $0 max_threads available_size\(MiB\) step_of_dup_rate
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
dup_arr=( $(seq 0 $3 100) )
echo dup_rate numjobs each\(MiB\) latency\(nsec\)
for dup in ${dup_arr[@]}; do
	for i in $(seq 1 $1); do
		each=$(($2 / $i))
		echo -n "$dup $i $each "
		bash latency_avg.sh $i ${each}M $dup | grep avg= | grep -v clat | awk '{print $5" "$2}' | sed 's/avg=//g' | sed 's/,//g' | sed 's/://g' | ./to_nsec
		echo
	done
done

