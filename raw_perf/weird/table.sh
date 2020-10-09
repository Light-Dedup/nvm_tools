make
TEMPOUT=$(mktemp)
echo "threads READ(GiB/s) WRITE(GiB/s)"
for i in $(seq 1 $1); do
	bash raw_perf.sh $i &> $TEMPOUT
	echo -n "$i "
	grep READ: $TEMPOUT | sed 's/.*READ: bw=//g' | sed 's/iB.*//g' | ./M2G | tr -d "\n"
	echo -n " "
	grep WRITE: $TEMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./M2G | tr -d "\n"
	echo
done
rm $TEMPOUT

