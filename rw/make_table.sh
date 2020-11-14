set -e

if [ ! $3 ]; then
	echo Usage: $0 max_threads size step_of_read_rate
	exit
fi
cd ..
make 1>&2
cd - > /dev/null
sudo bash -c "echo $0 $* > /dev/kmsg"
read_arr=( $(seq 0 $3 100) )
TMPOUT=$(mktemp)
echo read_rate numjobs write\(GiB/s\) read\(GiB/s\)
for rate in ${read_arr[@]}; do
	for i in $(seq 1 $1); do
		echo -n "$rate $i "
		bash rw.sh $i $2 $rate > $TMPOUT
		cd ..
		grep WRITE: $TMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum | tr -d "\n"
		echo -n " "
		grep READ: $TMPOUT | sed 's/.*READ: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum
		echo
		cd - > /dev/null
	done
done
rm $TMPOUT

