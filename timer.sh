set -e

cd ..
make -j$(nproc) 1>&2
sudo bash -c "echo $0 $* > /dev/kmsg"
sudo bash setup.sh 1 1>&2
cd - > /dev/null
bash helper/fio.sh $* 1>&2

cd ..
sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test 1>&2
cd - > /dev/null
echo "Type Count Total(ns)"
bash helper/timing.sh incr_ref
bash helper/timing.sh fp_calc
bash helper/timing.sh split
bash helper/timing.sh mem_bucket_find
bash helper/timing.sh memcmp
bash helper/timing.sh memcpy_data_block
bash helper/timing.sh write_new_entry
