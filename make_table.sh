if [ ! $3 ]; then
	echo Usage: $0 max_threads size step_of_dup_rate
	exit
fi
sudo bash -c "echo make_table $* > /dev/kmsg"
dup_arr=( $(seq 0 $3 100) )
echo dup_rate numjobs throughput\(GiB/s\)
for dup in ${dup_arr[@]}; do
	for i in $(seq 1 $1); do
		echo -n "$dup $i "
		bash timing_concurrent.sh $i $2 $dup | grep Total: | sed 's,Total:,,g' | sed 's,GiB/s,,g'
	done
done

