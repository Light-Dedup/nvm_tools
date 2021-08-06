set -e

if [ ! $4 ]; then
	echo Usage: $0 max_threads available_size\(MiB\) step_of_dup_rate pre_process_path
	exit
fi
sudo bash -c "echo $0 $* > /dev/kmsg"
dup_arr=( $(seq 0 $3 100) )
echo dup_rate numjobs each\(MiB\) latency\(nsec\)
for dup in ${dup_arr[@]}; do
	for ((i=1;i<=$1;i=i*2));  do
		each=$(($2 / $i))
		echo -n "$dup $i $each "
    bash $4 1>&2
		bash helper/fio.sh $i ${each}M $dup | grep avg= | grep -v clat | awk -F '=' '{print $1" "$2" "$3" "$4" "$5}' | awk '{print $8" "$2}'  | sed 's/,//g' | sed 's/://g'| ./to_nsec
		echo
	done
done

