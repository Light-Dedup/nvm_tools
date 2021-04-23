set -e
cd ..
make -j$(nproc)
sudo bash setup.sh 1
sudo fio -filename=/mnt/pmem/test1 -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=128G -name=randrw --dedupe_percentage=0 -group_reporting
sudo bash remount.sh
sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
dmesg | grep save_refcount | tail -n 1
dmesg | grep save_entry_allocator | tail -n 1
dmesg | grep normal_recover_fp_table | tail -n 1
dmesg | grep normal_recover_entry_allocator | tail -n 1

