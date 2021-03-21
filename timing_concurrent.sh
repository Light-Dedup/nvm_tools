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

TMPOUT=$(mktemp)
bash helper/fio.sh $* | tee $TMPOUT

make
echo -n Total:
grep WRITE: $TMPOUT | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./toG | ./get_sum | tr -d "\n"
echo GiB/s

rm $TMPOUT

#cd ..
#sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
#sudo dmesg | tail -n 50

