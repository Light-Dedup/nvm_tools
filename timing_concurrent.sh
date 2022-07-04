set -e

if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate [block_size]
	exit 1
fi
cd ..
sudo bash -c "echo $0 $* > /dev/kmsg"
bash setup.sh
cd -

bash helper/fio.sh $*
