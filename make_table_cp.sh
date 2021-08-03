set -e
if [ ! $3 ]; then
	echo Usage: $0 max_threads src_dir dst_dir
	exit
fi
ori=$(pwd)
echo Threads First\(s\) Second\(s\)
for i in $(seq 1 $1); do
	echo -n "$i "
	rm -rf $3
	( /bin/time -f %e bash -c "ulimit -n 1000000 && mcp $2 $3 $i" 3>&1 1>&2 2>&3 )
done
