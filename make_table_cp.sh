set -e
if [ ! $2 ]; then
	echo Usage: $0 max_threads source_dir
	exit
fi
ori=$(pwd)
echo Threads First\(s\) Second\(s\)
for i in $(seq 1 $1); do
	echo -n "$i "
	cd ..
	umount /mnt/pmem || true
	umount /mnt/source || true
	mount -t NOVA -o init -o wprotect,data_cow /dev/pmem0 /mnt/pmem
	mount -t NOVA -o init -o wprotect,data_cow /dev/pmem1 /mnt/source
	cd $2
	#find . -type f -o -type l | $ori/helper/split_dir.py $i /mnt/source
	find . -type f | $ori/helper/split_dir.py $i /mnt/source
	cd /mnt/source
	du -hd 1 1>&2
	mkdir /mnt/pmem/0
	/bin/time -f %e bash -c "ls | xargs -P $i -I {} cp -r {} /mnt/pmem/0/" |& xargs echo -n
	echo -n " "
	mkdir /mnt/pmem/1
	cd /mnt/pmem/0
	/bin/time -f %e bash -c "ls | xargs -P $i -I {} cp -r {} /mnt/pmem/1/" 2>&1
	cd $ori
done
