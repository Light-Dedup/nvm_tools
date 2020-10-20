if [ ! $2 ]; then
	echo Usage: $0 max_threads path_to_init_shell_script
	exit
fi

TEMPOUT=$(mktemp)
echo "threads READ(GiB/s) WRITE(GiB/s)"
for i in $(seq 1 $1); do
	bash raw_perf.sh $i $2 &> $TEMPOUT
	echo -n "$i "
	grep Read: $TEMPOUT | sed 's/Read://g' | sed 's/GiB.*//g' | tr -d "\n"
	echo -n " "
	grep Write: $TEMPOUT | sed 's/Write://g' | sed 's/GiB.*//g' | tr -d "\n"
	echo
done
rm $TEMPOUT

