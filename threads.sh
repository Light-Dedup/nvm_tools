for i in $(seq 1 $1); do
	echo -n "$i "
	bash timing_concurrent.sh $i $2 | grep Total: | sed 's,Total:,,g' | sed 's,MiB/s,,g'
done

