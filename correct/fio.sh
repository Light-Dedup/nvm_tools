set -e
if [[ $# < 4 || $# > 5 ]]; then
	echo Usage: $0 pmem-num num_of_threads size duplicate_percentage [block_size]
	exit 1
fi
if [ ! $5 ]; then
	bs=4K
else
	bs=$5
fi

fs_dir=$(pwd)
tools_dir=$(dirname $(realpath $0))/..

cd $tools_dir
make -j$(nproc)
bash helper/mkstdfile.sh $*
cd $fs_dir
bash $tools_dir/setup/nova.sh $1
cd $tools_dir
bash helper/fio.sh $2 $3 $4 $bs

STD_DIR=$HOME/fs_test/test_$(echo $2 $3 $4 $bs | sed 's/ /_/g')
bash helper/cmpstdfile.sh $STD_DIR /mnt/pmem$1
