TEMPOUT=$(mktemp)
echo "threads READ(GiB/s) WRITE(GiB/s)"
for i in $(seq 1 $1); do
	bash raw_perf.sh $i &> $TEMPOUT
	echo -n "$i "
	grep Read: $TEMPOUT | sed 's/Read://g' | sed 's/GiB.*//g' | tr -d "\n"
	echo -n " "
	grep Write: $TEMPOUT | sed 's/Write://g' | sed 's/GiB.*//g' | tr -d "\n"
	echo
done
rm $TEMPOUT

