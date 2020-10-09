#TEMPOUT=$(mktemp)
TEMPOUT=out.txt
echo "threads READ WRITE"
for i in $(seq 1 $1); do
	bash raw_perf.sh $i &> $TEMPOUT
	echo -n "$i "
	grep READ: $TEMPOUT | sed 's/.*READ: bw=//g' | sed 's/MiB.*//g' | tr -d "\n"
	echo -n " "
	grep WRITE: $TEMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/MiB.*//g' | tr -d "\n"
	echo
done
#rm $TEMPOUT

