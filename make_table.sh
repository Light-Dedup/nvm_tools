if [ ! $2 ]; then
	echo Usage: $0 max_threads size
	exit
fi
dup_arr=( $(seq 0 100) )
for dup in ${dup_arr[@]}; do
	for i in $(seq 1 $1); do
		echo -n "$dup $i "
		bash timing_concurrent.sh $i $2 $dup | grep Total: | sed 's,Total:,,g' | sed 's,MiB/s,,g'
	done
done

