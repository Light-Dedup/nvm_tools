set -e
if [[ $# != 5 ]]; then
	echo Usage: $0 pmem-num thread-num size dup-percentage block-size
	exit 1
fi
tools_dir=$(pwd)/$(dirname $0)/..
nova_dir=$(pwd)
std_dir=$HOME/fs_test/test_$(echo $2 $3 $4 $5 | sed 's/ /_/g')

cd $tools_dir
make -j$(nproc)
bash helper/mkstdfile.sh $*
cd $nova_dir
bash $tools_dir/setup/nova.sh $1
cd $tools_dir
bash helper/fio.sh $2 $3 $4 $5

sudo umount /mnt/pmem$1
sudo mount -t NOVA -o data_cow /dev/pmem$1 /mnt/pmem$1
bash helper/cmpstdfile.sh $std_dir /mnt/pmem$1
