if [ ! $4 ]; then
	echo Usage: $0 path_to_init_script max_threads size step_of_read_rate
	exit
fi
cd ..
make
cd -
sudo bash -c "echo $0 $* > /dev/kmsg"
read_arr=( $(seq 80 $4 100) )
TMPOUT=$(mktemp)
echo read_rate numjobs write\(GiB/s\) read\(GiB/s\)
for rate in ${read_arr[@]}; do
	for i in $(seq 1 $2); do
		echo -n "$rate $i "
		bash raw_perf.sh $1 $i $3 $rate > $TMPOUT
		cd ..
		grep WRITE: $TMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum | tr -d "\n"
		echo -n " "
		grep READ: $TMPOUT | sed 's/.*READ: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum
		echo
		cd - > /dev/null
	done
done
rm $TMPOUT

