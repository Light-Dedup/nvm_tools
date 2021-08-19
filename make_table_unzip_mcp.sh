set -e
if [ ! $4 ]; then
	echo Usage: $0 max_threads src_zipfile dst_dir pre_process_path
	exit
fi

echo Threads unzip\(s\) mcp\(s\)
for ((i=1;i<=$1;i=i*2)); do
	echo -n "$i "
	bash $4 1>&2
	cp $2 $3
	cd $3
	mkdir a
	mkdir b
	echo -n $(/bin/time -f %e bash -c "pbzip2 -cdkp16 $(basename $2) | tar -xC $3/a" 2>&1)
	echo -n " "
	cd /home/cyz/nova/tools
	( /bin/time -f %e bash -c "ulimit -n 1000000 && mcp $3/a $3/b $i" 3>&1 1>&2 2>&3 )
done
