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
bash $tools_dir/setup/nova.sh $1 1
cd $tools_dir
bash helper/fio.sh $2 $3 $4 $5

sudo mkdir /mnt/pmem$1/0
sudo mv /mnt/pmem$1/test* /mnt/pmem$1/0/

bash helper/fio.sh $2 $3 $4 $5
sudo rm -r /mnt/pmem$1/0/
bash helper/cmpstdfile.sh $std_dir /mnt/pmem$1/
sudo rm /mnt/pmem$1/*
sleep 1
df -h
