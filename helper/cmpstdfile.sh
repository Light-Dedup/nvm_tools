set -e
if [[ $# != 2 ]]; then
    echo Usage: $0 std_dir test_dir
    exit
fi
for file in $(ls $2); do
	if [ -f $file ]; then
		echo cmp $1/$file $2/$file
		cmp $1/$file $2/$file
	fi
done
