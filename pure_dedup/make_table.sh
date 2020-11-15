set -e

if [ ! $2 ]; then
	echo Usage: $0 max_threads size
	exit
fi
cd ..
make 1>&2
cd - > /dev/null
sudo bash -c "echo $0 $* > /dev/kmsg"
TMPOUT=$(mktemp)
echo numjobs throughput\(GiB/s\)
for i in $(seq 1 $1); do
	echo -n "$i "
	bash pure_dedup.sh $i $2 > $TMPOUT
	cd ..
	grep WRITE: $TMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum
	echo
	cd - > /dev/null
done
rm $TMPOUT

