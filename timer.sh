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
echo type nsec
echo fp_calc $(bash helper/timing.sh fp_calc)
echo split $(bash helper/timing.sh split)
echo memcmp $(bash helper/timing.sh memcmp)
echo memcpy_data_block $(bash helper/timing.sh memcpy_data_block)
echo write_new_entry $(bash helper/timing.sh write_new_entry)
echo incr_ref $(bash helper/timing.sh incr_ref)
