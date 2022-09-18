set -e
if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size duplicate_percentage [block_size]
	exit 1
fi

cd ..
make
bash mkstdfile.sh $*

bash timing_concurrent.sh $*

TESTDIR=$HOME/fs_test/test_$(echo $* | sed 's/ /_/g')
bash cmpstdfile.sh $TESTDIR
