set -e
if [ ! $1 ]; then
    echo Usage: $0 std_dir
    exit
fi
if [ $2 ]; then
	test_dir=$2
else
	test_dir=/mnt/pmem
fi
for file in $(ls $test_dir); do
	echo cmp $1/$file $test_dir/$file
	cmp $1/$file $test_dir/$file
done
