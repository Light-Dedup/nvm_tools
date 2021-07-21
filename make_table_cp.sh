if [ ! $2 ]; then
	echo Usage: $0 max_threads source_dir
	exit
fi
ori=$(pwd)
echo Threads First\(s\) Second\(s\)
for i in $(seq 1 $1); do
	echo -n "$i "
	cd ..
	bash setup.sh 1>&2
	cd $2
	mkdir /mnt/pmem/0
	/bin/time -f %e bash -c "find . -type f | xargs -P $i -I {} cp --parents {} /mnt/pmem/0/" |& xargs echo -n
	echo -n " "
	mkdir /mnt/pmem/1
	cd /mnt/pmem/0
	/bin/time -f %e bash -c "find . -type f | xargs -P $i -I {} cp --parents {} /mnt/pmem/1/" 2>&1
	cd $ori
done
