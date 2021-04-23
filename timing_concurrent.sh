set -e

if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate
	exit 1
fi
cd ..
make
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh
cd -

bash helper/fio.sh $*
