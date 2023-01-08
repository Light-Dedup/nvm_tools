if [[ $# != 5 ]]; then
	echo Usage: $0 pmem-num thread-num size dup-percentage block-size
	exit 1
fi
tools_dir=$(pwd)/$(dirname $0)/..
nova_dir=$(pwd)

cd $tools_dir
make -j$(nproc)
bash mkstdfile.sh $2 $3 $4 $5

cd $nova_dir
bash setup.sh 1

cd $tools_dir
bash helper/fio.sh $2 $3 $4 $5
sudo umount /mnt/pmem
sudo mount -t NOVA -o data_cow /dev/pmem$1 /mnt/pmem
TESTDIR=$HOME/fs_test/test_$(echo $2 $3 $4 $5 | sed 's/ /_/g')
bash cmpstdfile.sh $TESTDIR /mnt/pmem
