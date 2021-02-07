set -e
if [ ! $1 ]; then
    echo Usage: $0 std_dir
    exit
fi
for file in $(ls /mnt/pmem); do
	echo cmp $1/$file /mnt/pmem/$file
	cmp $1/$file /mnt/pmem/$file
done
